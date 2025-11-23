#ifndef TCP_HANDLER_H
#define TCP_HANDLER_H

#include "../protocol.h"

void send_tcp_message(int fd, MessageType type, const char* json_data);
void broadcast_to_room(Room* room, MessageType type, const char* json_data, Player* exclude);
void handle_tcp_message(Player* player, const char* buffer, int len);
void handle_disconnect(Player* player);

#endif // TCP_HANDLER_H
