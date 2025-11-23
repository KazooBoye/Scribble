#ifndef JSON_H
#define JSON_H

#include "../protocol.h"

char* json_create_message(MessageType type, const char* data);
int json_get_string(const char* json, const char* key, char* out, int out_size);
int json_get_int(const char* json, const char* key, int* out);
int json_get_type(const char* json, MessageType* type);

#endif // JSON_H
