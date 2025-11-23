#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <pthread.h>
#include <stdbool.h>

int udp_server_start(int port);
void udp_server_stop();

#endif // UDP_SERVER_H
