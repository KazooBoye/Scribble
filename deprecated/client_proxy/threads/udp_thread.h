#ifndef UDP_THREAD_H
#define UDP_THREAD_H

#include "../threads/dispatcher.h"
#include <pthread.h>

typedef struct {
    Dispatcher* dispatcher;
    volatile bool running;
    pthread_t thread;
    char server_host[256];
    int server_port;
} UDPThread;

int udp_thread_init(UDPThread* udp, Dispatcher* dispatcher, const char* host, int port);
void udp_thread_destroy(UDPThread* udp);
void* udp_thread_func(void* arg);

#endif // UDP_THREAD_H
