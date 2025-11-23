#ifndef JSON_H
#define JSON_H

#include "../protocol.h"

// JSON creation helpers
char* json_create_message(MessageType type, const char* data);
char* json_create_simple(MessageType type, const char* key, const char* value);
char* json_create_error(const char* error_msg);
char* json_create_player_info(const Player* player);
char* json_create_room_state(const Room* room);

// Simple JSON parsing helpers
int json_get_string(const char* json, const char* key, char* out, int out_size);
int json_get_int(const char* json, const char* key, int* out);
int json_get_type(const char* json, MessageType* type);

#endif // JSON_H
