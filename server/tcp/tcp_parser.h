#ifndef TCP_PARSER_H
#define TCP_PARSER_H

#include "../protocol.h"

int serialize_tcp_message(MessageType type, const char* json, char* buffer, int buffer_size);
int deserialize_tcp_message(const char* buffer, int len, MessageType* type, char** json);

#endif // TCP_PARSER_H
