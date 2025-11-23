#ifndef STATE_CACHE_H
#define STATE_CACHE_H

#include "../protocol.h"
#include <pthread.h>

typedef struct {
    uint32_t room_id;
    uint32_t player_id;
    char username[MAX_USERNAME];
    int time_remaining;
    int player_count;
    Stroke last_strokes[100];
    int last_stroke_count;
    bool valid;
    pthread_mutex_t mutex;
} ProxyState;

void state_cache_init(ProxyState* state);
void state_cache_destroy(ProxyState* state);
void state_cache_update_room(ProxyState* state, uint32_t room_id);
void state_cache_update_player(ProxyState* state, uint32_t player_id, const char* username);
void state_cache_update_timer(ProxyState* state, int time_remaining);
void state_cache_add_stroke(ProxyState* state, const Stroke* stroke);

#endif // STATE_CACHE_H
