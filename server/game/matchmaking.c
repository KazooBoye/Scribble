#include "matchmaking.h"
#include "../utils/logger.h"
#include "../utils/json.h"
#include "../utils/timer.h"
#include "../tcp/tcp_handler.h"
#include "game_logic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LATENCY_TOLERANCE 50  // ms

static Room rooms[MAX_ROOMS];
static Player* waiting_queue[MAX_ROOMS];
static int queue_size = 0;
static uint32_t next_room_id = 1;
static pthread_mutex_t matchmaking_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_matchmaking() {
    memset(rooms, 0, sizeof(rooms));
    memset(waiting_queue, 0, sizeof(waiting_queue));
    queue_size = 0;
}

Room* find_room_by_id(uint32_t room_id) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].room_id == room_id && rooms[i].player_count > 0) {
            return &rooms[i];
        }
    }
    return NULL;
}

void iterate_active_rooms(void (*callback)(Room*)) {
    pthread_mutex_lock(&matchmaking_mutex);
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].player_count > 0 && 
            (rooms[i].state == ROOM_PLAYING || rooms[i].state == ROOM_WAITING)) {
            callback(&rooms[i]);
        }
    }
    pthread_mutex_unlock(&matchmaking_mutex);
}

Room* find_room_by_code(const char* code) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].is_private && 
            rooms[i].player_count > 0 && 
            strcmp(rooms[i].room_code, code) == 0) {
            return &rooms[i];
        }
    }
    return NULL;
}

Room* create_private_room() {
    pthread_mutex_lock(&matchmaking_mutex);
    
    // Find empty room slot
    Room* room = NULL;
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].player_count == 0) {
            room = &rooms[i];
            init_room(room, next_room_id++, true);
            break;
        }
    }
    
    pthread_mutex_unlock(&matchmaking_mutex);
    return room;
}

int join_private_room(Player* player, const char* room_code) {
    pthread_mutex_lock(&matchmaking_mutex);
    
    Room* room = find_room_by_code(room_code);
    if (!room) {
        pthread_mutex_unlock(&matchmaking_mutex);
        return -1;  // Room not found
    }
    
    if (room->player_count >= MAX_PLAYERS) {
        pthread_mutex_unlock(&matchmaking_mutex);
        return -2;  // Room full
    }
    
    add_player_to_room(room, player);
    
    // Start game if room is full
    if (room->player_count == MAX_PLAYERS) {
        start_game(room);
    }
    
    pthread_mutex_unlock(&matchmaking_mutex);
    return 0;
}

int join_matchmaking(Player* player) {
    pthread_mutex_lock(&matchmaking_mutex);
    
    // Try to find a suitable room based on latency
    Room* best_room = NULL;
    int best_latency_diff = LATENCY_TOLERANCE;
    
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].player_count > 0 && 
            rooms[i].player_count < MAX_PLAYERS &&
            !rooms[i].is_private &&
            rooms[i].state == ROOM_WAITING &&
            rooms[i].round_number <= 1) {
            
            // Calculate average latency of room
            uint64_t avg_latency = 0;
            for (int j = 0; j < rooms[i].player_count; j++) {
                if (rooms[i].players[j]) {
                    avg_latency += rooms[i].players[j]->rtt;
                }
            }
            avg_latency /= rooms[i].player_count;
            
            int latency_diff = abs((int)(player->rtt - avg_latency));
            
            if (latency_diff < best_latency_diff) {
                best_latency_diff = latency_diff;
                best_room = &rooms[i];
            }
        }
    }
    
    // If no suitable room found, create new one
    if (!best_room) {
        for (int i = 0; i < MAX_ROOMS; i++) {
            if (rooms[i].player_count == 0) {
                best_room = &rooms[i];
                init_room(best_room, next_room_id++, false);
                break;
            }
        }
    }
    
    if (!best_room) {
        pthread_mutex_unlock(&matchmaking_mutex);
        return -1;  // No rooms available
    }
    
    add_player_to_room(best_room, player);
    
    // Start countdown timer when 2nd player joins
    if (best_room->player_count == 2 && !best_room->countdown_active) {
        best_room->countdown_active = true;
        best_room->game_start_countdown = get_current_time_ms();
        printf("[COUNTDOWN] Room %u countdown started with %d players at timestamp %llu\n", 
               best_room->room_id, best_room->player_count, best_room->game_start_countdown);
        log_room_event(best_room->room_id, "countdown_started", "15s until game starts");
    }
    
    // Start game immediately if room is full
    if (best_room->player_count == MAX_PLAYERS && best_room->state == ROOM_WAITING) {
        best_room->countdown_active = false;
        start_game(best_room);
        
        // Notify all players with MSG_GAME_START
        char* game_state = json_create_room_state(best_room);
        broadcast_to_room(best_room, MSG_GAME_START, game_state, NULL);
        free(game_state);
        
        // Send the actual word to the drawer
        if (best_room->players[best_room->current_drawer_idx]) {
            char word_msg[256];
            snprintf(word_msg, sizeof(word_msg), 
                     "{\"word\":\"%s\"}", best_room->current_word);
            send_tcp_message(best_room->players[best_room->current_drawer_idx]->fd, 
                           MSG_WORD_TO_DRAW, word_msg);
        }
    }
    
    pthread_mutex_unlock(&matchmaking_mutex);
    return 0;
}

void leave_room(Player* player) {
    pthread_mutex_lock(&matchmaking_mutex);
    
    for (int i = 0; i < MAX_ROOMS; i++) {
        for (int j = 0; j < rooms[i].player_count; j++) {
            if (rooms[i].players[j] == player) {
                remove_player_from_room(&rooms[i], player);
                
                // If room is empty, reset it
                if (rooms[i].player_count == 0) {
                    memset(&rooms[i], 0, sizeof(Room));
                }
                
                pthread_mutex_unlock(&matchmaking_mutex);
                return;
            }
        }
    }
    
    pthread_mutex_unlock(&matchmaking_mutex);
}

Room* get_player_room(Player* player) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        for (int j = 0; j < rooms[i].player_count; j++) {
            if (rooms[i].players[j] == player) {
                return &rooms[i];
            }
        }
    }
    return NULL;
}
