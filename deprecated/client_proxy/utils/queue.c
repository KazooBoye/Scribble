#include "queue.h"
#include <stdlib.h>
#include <string.h>

void queue_init(MessageQueue* queue) {
    memset(queue, 0, sizeof(MessageQueue));
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
}

void queue_destroy(MessageQueue* queue) {
    pthread_mutex_lock(&queue->mutex);
    
    // Free all pending messages
    for (int i = 0; i < queue->count; i++) {
        int idx = (queue->head + i) % QUEUE_SIZE;
        if (queue->items[idx].data) {
            free(queue->items[idx].data);
        }
    }
    
    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
}

bool queue_push(MessageQueue* queue, const char* data, int len, int client_id) {
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->count >= QUEUE_SIZE) {
        pthread_mutex_unlock(&queue->mutex);
        return false;  // Queue full
    }
    
    Message* msg = &queue->items[queue->tail];
    msg->data = malloc(len);
    if (!msg->data) {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    
    memcpy(msg->data, data, len);
    msg->len = len;
    msg->client_id = client_id;
    
    queue->tail = (queue->tail + 1) % QUEUE_SIZE;
    queue->count++;
    
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
    
    return true;
}

bool queue_pop(MessageQueue* queue, Message* msg) {
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->count == 0) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    
    *msg = queue->items[queue->head];
    queue->head = (queue->head + 1) % QUEUE_SIZE;
    queue->count--;
    
    pthread_mutex_unlock(&queue->mutex);
    
    return true;
}

int queue_size(MessageQueue* queue) {
    pthread_mutex_lock(&queue->mutex);
    int size = queue->count;
    pthread_mutex_unlock(&queue->mutex);
    return size;
}
