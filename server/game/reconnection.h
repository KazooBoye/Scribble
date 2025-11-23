#ifndef RECONNECTION_H
#define RECONNECTION_H

#include "../protocol.h"
#include <pthread.h>

void init_reconnection();
void generate_session_token(char* token, uint32_t player_id);
void save_player_state(Player* player, Room* room);
int restore_player_state(Player* player, const char* session_token, Room** out_room);
void cleanup_expired_states();

#endif // RECONNECTION_H
