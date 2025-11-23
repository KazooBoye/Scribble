#include "state_cache.h"
#include <string.h>

void state_cache_init(ProxyState* state) {
    memset(state, 0, sizeof(ProxyState));
    pthread_mutex_init(&state->mutex, NULL);
}

void state_cache_destroy(ProxyState* state) {
    pthread_mutex_destroy(&state->mutex);
}

void state_cache_update_room(ProxyState* state, uint32_t room_id) {
    pthread_mutex_lock(&state->mutex);
    state->room_id = room_id;
    state->valid = true;
    pthread_mutex_unlock(&state->mutex);
}

void state_cache_update_player(ProxyState* state, uint32_t player_id, const char* username) {
    pthread_mutex_lock(&state->mutex);
    state->player_id = player_id;
    if (username) {
        strncpy(state->username, username, MAX_USERNAME - 1);
        state->username[MAX_USERNAME - 1] = '\0';
    }
    pthread_mutex_unlock(&state->mutex);
}

void state_cache_update_timer(ProxyState* state, int time_remaining) {
    pthread_mutex_lock(&state->mutex);
    state->time_remaining = time_remaining;
    pthread_mutex_unlock(&state->mutex);
}

void state_cache_add_stroke(ProxyState* state, const Stroke* stroke) {
    pthread_mutex_lock(&state->mutex);
    if (state->last_stroke_count < 100) {
        state->last_strokes[state->last_stroke_count] = *stroke;
        state->last_stroke_count++;
    }
    pthread_mutex_unlock(&state->mutex);
}
