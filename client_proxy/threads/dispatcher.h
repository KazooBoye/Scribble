#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "../protocol.h"
#include "../utils/queue.h"
#include "../utils/state_cache.h"
#include <pthread.h>

typedef struct {
    MessageQueue from_ws_queue;     // Messages from WebSocket to server
    MessageQueue to_ws_queue;       // Messages from server to WebSocket
    MessageQueue from_tcp_queue;    // Messages from TCP server
    MessageQueue to_tcp_queue;      // Messages to TCP server
    MessageQueue from_udp_queue;    // Messages from UDP server
    MessageQueue to_udp_queue;      // Messages to UDP server
    ProxyState state;
    volatile bool running;
    pthread_t thread;
} Dispatcher;

int dispatcher_init(Dispatcher* dispatcher);
void dispatcher_destroy(Dispatcher* dispatcher);
void* dispatcher_thread_func(void* arg);

#endif // DISPATCHER_H
