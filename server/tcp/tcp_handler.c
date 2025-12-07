#include "tcp_handler.h"
#include "tcp_parser.h"
#include "../utils/json.h"
#include "../utils/logger.h"
#include "../utils/timer.h"
#include "../game/matchmaking.h"
#include "../game/game_logic.h"
#include "../game/reconnection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static uint32_t next_player_id = 1;

void send_tcp_message(int fd, MessageType type, const char* json_data) {
    const char* type_names[] = {"PING", "PONG", "REGISTER", "REGISTER_ACK", "JOIN_ROOM", "CREATE_ROOM",
                                "ROOM_CREATED", "ROOM_JOINED", "ROOM_FULL", "ROOM_NOT_FOUND", "GAME_START",
                                "YOUR_TURN", "WORD_TO_DRAW", "ROUND_START", "CHAT", "CHAT_BROADCAST",
                                "GUESS_CORRECT", "GUESS_WRONG", "TIMER_UPDATE", "COUNTDOWN_UPDATE",
                                "ROUND_END", "GAME_END", "PLAYER_JOIN", "PLAYER_LEAVE", "SCORE_UPDATE"};
    const char* type_name = (type >= 0 && type < 25) ? type_names[type] : "UNKNOWN";
    
    char* json_msg = json_create_message(type, json_data);
    if (!json_msg) return;
    
    printf("[TCP] send_tcp_message: type=%d (%s), fd=%d, json_msg=%s\n", type, type_name, fd, json_msg);
    
    char buffer[BUFFER_SIZE];
    int len = serialize_tcp_message(type, json_msg, buffer, sizeof(buffer));
    
    if (len > 0) {
        send(fd, buffer, len, 0);
    }
    
    free(json_msg);
}

void broadcast_to_room(Room* room, MessageType type, const char* json_data, Player* exclude) {
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i] && room->players[i] != exclude) {
            send_tcp_message(room->players[i]->fd, type, json_data);
        }
    }
}

void handle_register(Player* player, const char* json) {
    char username[MAX_USERNAME];
    printf("[TCP] handle_register: json=%s\n", json);
    
    if (json_get_data_string(json, "username", username, sizeof(username)) < 0) {
        strcpy(username, "Player");
        printf("[TCP] handle_register: Failed to get username, using default\n");
    }
    
    player->player_id = next_player_id++;
    strncpy(player->username, username, MAX_USERNAME - 1);
    player->state = PLAYER_LOBBY;
    player->score = 0;
    player->last_seen = get_current_time_ms();
    
    generate_session_token(player->session_token, player->player_id);
    
    printf("[TCP] Registered player %u: %s (fd=%d)\n", player->player_id, username, player->fd);
    
    char response[512];
    snprintf(response, sizeof(response), 
             "{\"player_id\":%u,\"username\":\"%s\",\"session_token\":\"%s\"}",
             player->player_id, player->username, player->session_token);
    
    send_tcp_message(player->fd, MSG_REGISTER_ACK, response);
    log_player_event(player->player_id, "registered", username);
}

void handle_ping(Player* player) {
    uint64_t ping_time = get_current_time_ms();
    player->last_seen = ping_time;
    
    char response[128];
    snprintf(response, sizeof(response), "{\"timestamp\":%llu}", (unsigned long long)ping_time);
    send_tcp_message(player->fd, MSG_PONG, response);
}

void handle_join_room(Player* player) {
    printf("[TCP] Player %u (%s) joining room\n", player->player_id, player->username);
    
    if (join_matchmaking(player) == 0) {
        Room* room = get_player_room(player);
        printf("[TCP] Player %u joined room %u (now %d players)\n", 
               player->player_id, room ? room->room_id : 0, room ? room->player_count : 0);
        if (room) {
            // Send full room state to ALL players (including new player and existing players)
            char* room_state = json_create_room_state(room);
            send_tcp_message(player->fd, MSG_ROOM_JOINED, room_state);
            
            // Send updated room state to existing players so they see the new player
            broadcast_to_room(room, MSG_ROOM_JOINED, room_state, player);
            free(room_state);
            
            // Also send player join notification for chat message
            char player_info[256];
            snprintf(player_info, sizeof(player_info), 
                     "{\"player_id\":%u,\"username\":\"%s\"}",
                     player->player_id, player->username);
            broadcast_to_room(room, MSG_PLAYER_JOIN, player_info, player);
        }
    } else {
        send_tcp_message(player->fd, MSG_ERROR, "{\"error\":\"Failed to join room\"}");
    }
}

void handle_create_room(Player* player) {
    Room* room = create_private_room();
    if (room) {
        add_player_to_room(room, player);
        
        char response[256];
        snprintf(response, sizeof(response), 
                 "{\"room_id\":%u,\"room_code\":\"%s\"}",
                 room->room_id, room->room_code);
        send_tcp_message(player->fd, MSG_ROOM_CREATED, response);
    } else {
        send_tcp_message(player->fd, MSG_ERROR, "{\"error\":\"Failed to create room\"}");
    }
}

void handle_chat(Player* player, const char* json) {
    char message[MAX_CHAT_LEN];
    printf("[TCP] handle_chat from player %u: json=%s\n", player->player_id, json);
    
    if (json_get_data_string(json, "message", message, sizeof(message)) < 0) {
        printf("[TCP] Failed to extract message from JSON\n");
        return;
    }
    
    printf("[TCP] Chat message from %s: %s\n", player->username, message);
    
    Room* room = get_player_room(player);
    if (!room) {
        printf("[TCP] Player %u not in a room\n", player->player_id);
        return;
    }
    
    // Check if it's a guess
    if (room->state == ROOM_PLAYING && !player->is_drawing) {
        int result = process_guess(room, player, message);
        
        if (result == 1) {
            // Correct guess!
            char response[512];
            snprintf(response, sizeof(response),
                     "{\"player_id\":%u,\"username\":\"%s\",\"score\":%d}",
                     player->player_id, player->username, player->score);
            broadcast_to_room(room, MSG_GUESS_CORRECT, response, NULL);
            
            // Send score update
            char score_msg[256];
            snprintf(score_msg, sizeof(score_msg),
                     "{\"player_id\":%u,\"score\":%d}",
                     player->player_id, player->score);
            broadcast_to_room(room, MSG_SCORE_UPDATE, score_msg, NULL);
            return;
        }
    }
    
    // Broadcast chat message
    char broadcast[512];
    snprintf(broadcast, sizeof(broadcast),
             "{\"player_id\":%u,\"username\":\"%s\",\"message\":\"%s\"}",
             player->player_id, player->username, message);
    broadcast_to_room(room, MSG_CHAT_BROADCAST, broadcast, NULL);
}

void handle_reconnect(Player* player, const char* json) {
    char session_token[64];
    if (json_get_data_string(json, "session_token", session_token, sizeof(session_token)) < 0) {
        send_tcp_message(player->fd, MSG_RECONNECT_FAIL, "{\"error\":\"Invalid token\"}");
        return;
    }
    
    Room* room = NULL;
    if (restore_player_state(player, session_token, &room) == 0) {
        // Success
        char* room_state = json_create_room_state(room);
        send_tcp_message(player->fd, MSG_RECONNECT_SUCCESS, room_state);
        free(room_state);
        
        // Notify other players
        char player_info[256];
        snprintf(player_info, sizeof(player_info),
                 "{\"player_id\":%u,\"username\":\"%s\"}",
                 player->player_id, player->username);
        broadcast_to_room(room, MSG_PLAYER_JOIN, player_info, player);
        
        // Resend all strokes to catch up
        for (int i = 0; i < room->stroke_count; i++) {
            // Note: Strokes are sent via UDP, but we can trigger resend here
        }
    } else {
        send_tcp_message(player->fd, MSG_RECONNECT_FAIL, 
                        "{\"error\":\"Reconnection failed\"}");
    }
}

void handle_disconnect(Player* player) {
    Room* room = get_player_room(player);
    if (room && room->state == ROOM_PLAYING) {
        // Save state for reconnection
        save_player_state(player, room);
        
        // Notify others
        char player_info[256];
        snprintf(player_info, sizeof(player_info),
                 "{\"player_id\":%u,\"username\":\"%s\"}",
                 player->player_id, player->username);
        broadcast_to_room(room, MSG_PLAYER_LEAVE, player_info, player);
    }
    
    if (room) {
        leave_room(player);
    }
}

void handle_clear_canvas(Player* player) {
    Room* room = get_player_room(player);
    if (!room || room->state != ROOM_PLAYING) {
        return;
    }
    
    // Only allow drawing player to clear canvas
    if (!player->is_drawing) {
        printf("[TCP] CLEAR: Player %u tried to clear but is not the drawer\n", player->player_id);
        return;
    }
    
    printf("[TCP] CLEAR: Broadcasting clear canvas from player %u to room %u\n", player->player_id, room->room_id);
    
    // Broadcast clear to all other players
    broadcast_to_room(room, UDP_CLEAR_CANVAS, "{}", player);
}

void handle_stroke(Player* player, const char* json) {
    Room* room = get_player_room(player);
    if (!room || room->state != ROOM_PLAYING) {
        printf("[TCP] STROKE: Player %u not in playing room (room=%p, state=%d)\n", 
               player->player_id, (void*)room, room ? room->state : -1);
        return;
    }
    
    // Only allow drawing player to send strokes
    if (!player->is_drawing) {
        printf("[TCP] STROKE: Player %u tried to draw but is not the drawer\n", player->player_id);
        return;
    }
    
    printf("[TCP] STROKE: Received from player %u in room %u\n", player->player_id, room->room_id);
    printf("[TCP] STROKE: Raw JSON: %s\n", json);
    
    // Extract just the stroke data from {"type":100,"data":{stroke_data}}
    const char* data_start = strstr(json, "\"data\":");
    if (data_start) {
        data_start += 7; // Skip "data":
        // Skip whitespace
        while (*data_start == ' ') data_start++;
        
        // The data_start now points to the stroke object {...}
        // We need to extract it and add player_id
        // Find the closing brace for the data object
        int brace_count = 0;
        const char* p = data_start;
        const char* data_end = NULL;
        
        while (*p) {
            if (*p == '{') brace_count++;
            else if (*p == '}') {
                brace_count--;
                if (brace_count == 0) {
                    data_end = p + 1;
                    break;
                }
            }
            p++;
        }
        
        if (data_end) {
            // Extract the stroke data
            int stroke_len = data_end - data_start;
            char stroke_data[BUFFER_SIZE];
            snprintf(stroke_data, sizeof(stroke_data), "%.*s", stroke_len, data_start);
            
            // Remove the closing } and add player_id
            stroke_data[stroke_len - 1] = '\0';  // Remove }
            char stroke_with_id[BUFFER_SIZE];
            snprintf(stroke_with_id, sizeof(stroke_with_id), 
                     "%s,\"player_id\":%u}", stroke_data, player->player_id);
            
            printf("[TCP] STROKE: Broadcasting data: %s\n", stroke_with_id);
            printf("[TCP] STROKE: Room has %d players, excluding sender (player_id=%u)\n", 
                   room->player_count, player->player_id);
            
            // Log each recipient
            for (int i = 0; i < room->player_count; i++) {
                if (room->players[i] && room->players[i] != player) {
                    printf("[TCP] STROKE: -> Sending to player %u (%s)\n",
                           room->players[i]->player_id, room->players[i]->username);
                }
            }
            
            // Broadcast - send_tcp_message will wrap it as {"type":100,"data":...}
            broadcast_to_room(room, UDP_STROKE, stroke_with_id, player);
        } else {
            printf("[TCP] STROKE: ERROR - Could not find data end\n");
        }
    } else {
        printf("[TCP] STROKE: WARNING - No data field found in JSON\n");
    }
}

void handle_tcp_message(Player* player, const char* buffer, int len) {
    MessageType type;
    char* json = NULL;
    
    int consumed = deserialize_tcp_message(buffer, len, &type, &json);
    if (consumed < 0 || !json) {
        printf("[TCP] Failed to deserialize message from player %u\n", player->player_id);
        return;
    }
    
    printf("[TCP] handle_tcp_message: player=%u, type=%d, json=%s\n", player->player_id, type, json);
    
    switch (type) {
        case MSG_REGISTER:
            handle_register(player, json);
            break;
        case MSG_PING:
            handle_ping(player);
            break;
        case MSG_JOIN_ROOM:
            handle_join_room(player);
            break;
        case MSG_CREATE_ROOM:
            handle_create_room(player);
            break;
        case MSG_CHAT:
            handle_chat(player, json);
            break;
        case MSG_RECONNECT_REQUEST:
            handle_reconnect(player, json);
            break;
        case MSG_DISCONNECT:
            handle_disconnect(player);
            break;
        case UDP_STROKE:
            handle_stroke(player, json);
            break;
        case UDP_CLEAR_CANVAS:
            handle_clear_canvas(player);
            break;
        default:
            printf("[TCP] Unknown message type %d from player %u\n", type, player->player_id);
            break;
    }
    
    free(json);
}
