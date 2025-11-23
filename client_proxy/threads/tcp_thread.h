#ifndef TCP_THREAD_H
#define TCP_THREAD_H

#include "../threads/dispatcher.h"
#include <pthread.h>

typedef struct {
    Dispatcher* dispatcher;
    volatile bool running;
    pthread_t thread;
    char server_host[256];
    int server_port;
} TCPThread;

int tcp_thread_init(TCPThread* tcp, Dispatcher* dispatcher, const char* host, int port);
void tcp_thread_destroy(TCPThread* tcp);
void* tcp_thread_func(void* arg);

#endif // TCP_THREAD_H
