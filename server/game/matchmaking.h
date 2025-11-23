#ifndef MATCHMAKING_H
#define MATCHMAKING_H

#include "../protocol.h"
#include <pthread.h>

void init_matchmaking();
Room* find_room_by_id(uint32_t room_id);
Room* find_room_by_code(const char* code);
Room* create_private_room();
int join_private_room(Player* player, const char* room_code);
int join_matchmaking(Player* player);
void leave_room(Player* player);
Room* get_player_room(Player* player);
void iterate_active_rooms(void (*callback)(Room*));

#endif // MATCHMAKING_H
