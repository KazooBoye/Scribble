#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "../protocol.h"
#include <pthread.h>
#include <stdbool.h>

int tcp_server_start(int port);
void tcp_server_stop();
void tcp_send_timer_updates(Room* room);
Player* find_player_by_ip(const char* ip);

#endif // TCP_SERVER_H
