#include "reconnection.h"
#include "../utils/logger.h"
#include "../utils/timer.h"
#include "matchmaking.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_DISCONNECTED_PLAYERS 100

typedef struct {
    char session_token[64];
    uint32_t player_id;
    uint32_t room_id;
    PlayerState state;
    int score;
    bool is_drawing;
    bool has_guessed;
    uint64_t disconnect_time;
    bool valid;
} DisconnectedPlayerState;

static DisconnectedPlayerState disconnected_players[MAX_DISCONNECTED_PLAYERS];
static pthread_mutex_t reconnect_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_reconnection() {
    memset(disconnected_players, 0, sizeof(disconnected_players));
}

void generate_session_token(char* token, uint32_t player_id) {
    uint64_t timestamp = get_current_time_ms();
    snprintf(token, 64, "%u-%lu-%d", player_id, timestamp, rand() % 10000);
}

void save_player_state(Player* player, Room* room) {
    pthread_mutex_lock(&reconnect_mutex);
    
    // Find empty slot
    int slot = -1;
    for (int i = 0; i < MAX_DISCONNECTED_PLAYERS; i++) {
        if (!disconnected_players[i].valid) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        // No space, overwrite oldest
        slot = 0;
        uint64_t oldest_time = disconnected_players[0].disconnect_time;
        for (int i = 1; i < MAX_DISCONNECTED_PLAYERS; i++) {
            if (disconnected_players[i].disconnect_time < oldest_time) {
                oldest_time = disconnected_players[i].disconnect_time;
                slot = i;
            }
        }
    }
    
    DisconnectedPlayerState* state = &disconnected_players[slot];
    strncpy(state->session_token, player->session_token, 63);
    state->player_id = player->player_id;
    state->room_id = room->room_id;
    state->state = player->state;
    state->score = player->score;
    state->is_drawing = player->is_drawing;
    state->has_guessed = player->has_guessed;
    state->disconnect_time = get_current_time_ms();
    state->valid = true;
    
    log_disconnect(player->player_id, "connection_lost");
    
    pthread_mutex_unlock(&reconnect_mutex);
}

int restore_player_state(Player* player, const char* session_token, Room** out_room) {
    pthread_mutex_lock(&reconnect_mutex);
    
    // Find player state
    DisconnectedPlayerState* state = NULL;
    for (int i = 0; i < MAX_DISCONNECTED_PLAYERS; i++) {
        if (disconnected_players[i].valid && 
            strcmp(disconnected_players[i].session_token, session_token) == 0) {
            state = &disconnected_players[i];
            break;
        }
    }
    
    if (!state) {
        pthread_mutex_unlock(&reconnect_mutex);
        log_reconnect(0, session_token, false);
        return -1;  // Token not found
    }
    
    // Check if timeout
    uint64_t elapsed = (get_current_time_ms() - state->disconnect_time) / 1000;
    if (elapsed > RECONNECT_TIMEOUT) {
        state->valid = false;
        pthread_mutex_unlock(&reconnect_mutex);
        log_reconnect(state->player_id, session_token, false);
        return -2;  // Timeout
    }
    
    // Find room
    Room* room = find_room_by_id(state->room_id);
    if (!room) {
        state->valid = false;
        pthread_mutex_unlock(&reconnect_mutex);
        log_reconnect(state->player_id, session_token, false);
        return -3;  // Room no longer exists
    }
    
    // Restore player state
    player->player_id = state->player_id;
    player->score = state->score;
    player->state = state->state;
    player->is_drawing = state->is_drawing;
    player->has_guessed = state->has_guessed;
    strncpy(player->session_token, state->session_token, 63);
    
    // Add player back to room
    int added = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (room->players[i] == NULL) {
            room->players[i] = player;
            room->player_count++;
            added = 1;
            break;
        }
    }
    
    if (!added) {
        pthread_mutex_unlock(&reconnect_mutex);
        log_reconnect(state->player_id, session_token, false);
        return -4;  // Room full
    }
    
    *out_room = room;
    state->valid = false;  // Mark as used
    
    log_reconnect(player->player_id, session_token, true);
    pthread_mutex_unlock(&reconnect_mutex);
    
    return 0;
}

void cleanup_expired_states() {
    pthread_mutex_lock(&reconnect_mutex);
    
    uint64_t now = get_current_time_ms();
    for (int i = 0; i < MAX_DISCONNECTED_PLAYERS; i++) {
        if (disconnected_players[i].valid) {
            uint64_t elapsed = (now - disconnected_players[i].disconnect_time) / 1000;
            if (elapsed > RECONNECT_TIMEOUT) {
                disconnected_players[i].valid = false;
            }
        }
    }
    
    pthread_mutex_unlock(&reconnect_mutex);
}
