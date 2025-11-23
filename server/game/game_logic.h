#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "../protocol.h"

int load_word_list(const char* filename);
const char* get_random_word();
void init_room(Room* room, uint32_t room_id, bool is_private);
int add_player_to_room(Room* room, Player* player);
int remove_player_from_room(Room* room, Player* player);
void start_game(Room* room);
void start_next_round(Room* room);
void end_round(Room* room);
void end_game(Room* room);
int process_guess(Room* room, Player* player, const char* guess);
void update_timer(Room* room);
void add_stroke(Room* room, const Stroke* stroke);
void cleanup_word_list();

#endif // GAME_LOGIC_H
