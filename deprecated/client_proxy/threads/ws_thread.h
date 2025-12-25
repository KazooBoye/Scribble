#ifndef WS_THREAD_H
#define WS_THREAD_H

#include "../threads/dispatcher.h"
#include <pthread.h>

typedef struct {
    Dispatcher* dispatcher;
    volatile bool running;
    pthread_t thread;
    int port;
} WSThread;

int ws_thread_init(WSThread* ws, Dispatcher* dispatcher, int port);
void ws_thread_destroy(WSThread* ws);
void* ws_thread_func(void* arg);

#endif // WS_THREAD_H
