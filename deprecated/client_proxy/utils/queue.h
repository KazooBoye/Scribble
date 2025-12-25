#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stdbool.h>

#define QUEUE_SIZE 1000

typedef struct {
    char* data;
    int len;
    int client_id;  // For routing back to specific WebSocket client
} Message;

typedef struct {
    Message items[QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} MessageQueue;

void queue_init(MessageQueue* queue);
void queue_destroy(MessageQueue* queue);
bool queue_push(MessageQueue* queue, const char* data, int len, int client_id);
bool queue_pop(MessageQueue* queue, Message* msg);
int queue_size(MessageQueue* queue);

#endif // QUEUE_H
