#include "dispatcher.h"
#include "../utils/json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int dispatcher_init(Dispatcher* dispatcher) {
    memset(dispatcher, 0, sizeof(Dispatcher));
    
    queue_init(&dispatcher->from_ws_queue);
    queue_init(&dispatcher->to_ws_queue);
    queue_init(&dispatcher->from_tcp_queue);
    queue_init(&dispatcher->to_tcp_queue);
    queue_init(&dispatcher->from_udp_queue);
    queue_init(&dispatcher->to_udp_queue);
    
    state_cache_init(&dispatcher->state);
    
    dispatcher->running = true;
    
    if (pthread_create(&dispatcher->thread, NULL, dispatcher_thread_func, dispatcher) != 0) {
        perror("Failed to create dispatcher thread");
        return -1;
    }
    
    printf("[DISPATCHER] Thread started\n");
    return 0;
}

void dispatcher_destroy(Dispatcher* dispatcher) {
    dispatcher->running = false;
    pthread_join(dispatcher->thread, NULL);
    
    queue_destroy(&dispatcher->from_ws_queue);
    queue_destroy(&dispatcher->to_ws_queue);
    queue_destroy(&dispatcher->from_tcp_queue);
    queue_destroy(&dispatcher->to_tcp_queue);
    queue_destroy(&dispatcher->from_udp_queue);
    queue_destroy(&dispatcher->to_udp_queue);
    
    state_cache_destroy(&dispatcher->state);
    
    printf("[DISPATCHER] Thread stopped\n");
}

void* dispatcher_thread_func(void* arg) {
    Dispatcher* dispatcher = (Dispatcher*)arg;
    
    printf("[DISPATCHER] Main loop started\n");
    
    while (dispatcher->running) {
        // Process messages from WebSocket (browser) -> forward to TCP/UDP
        if (queue_size(&dispatcher->from_ws_queue) > 0) {
            Message msg;
            if (queue_pop(&dispatcher->from_ws_queue, &msg)) {
                printf("[DISPATCHER] Processing message from WS: %.*s\n", msg.len, msg.data);
                // Parse message to determine routing
                MessageType type;
                if (json_get_type(msg.data, &type) == 0) {
                    printf("[DISPATCHER] Message type: %d\n", type);
                    // Check if it's a drawing stroke (UDP) or other message (TCP)
                    if (type == UDP_STROKE) {
                        // Route to UDP
                        printf("[DISPATCHER] Routing to UDP\n");
                        queue_push(&dispatcher->to_udp_queue, msg.data, msg.len, msg.client_id);
                    } else {
                        // Route to TCP
                        printf("[DISPATCHER] Routing to TCP\n");
                        queue_push(&dispatcher->to_tcp_queue, msg.data, msg.len, msg.client_id);
                    }
                    
                    // Update state cache
                    if (type == MSG_REGISTER_ACK) {
                        int player_id;
                        char username[MAX_USERNAME];
                        if (json_get_int(msg.data, "player_id", &player_id) == 0 &&
                            json_get_string(msg.data, "username", username, sizeof(username)) == 0) {
                            state_cache_update_player(&dispatcher->state, player_id, username);
                        }
                    }
                }
                
                free(msg.data);
            }
        }
        
        // Process messages from TCP -> forward to WebSocket
        if (queue_size(&dispatcher->from_tcp_queue) > 0) {
            Message msg;
            if (queue_pop(&dispatcher->from_tcp_queue, &msg)) {
                printf("[DISPATCHER] Received from TCP: %d bytes\n", msg.len);
                
                // Deserialize length-prefixed message
                if (msg.len >= 4) {
                    uint32_t json_len;
                    memcpy(&json_len, msg.data, 4);
                    json_len = ntohl(json_len);
                    
                    if (msg.len >= 4 + (int)json_len) {
                        char* json_payload = msg.data + 4;
                        printf("[DISPATCHER] JSON from server: %.*s\n", (int)json_len, json_payload);
                        
                        // Forward JSON to WebSocket clients
                        queue_push(&dispatcher->to_ws_queue, json_payload, json_len, -1);
                        
                        // Update state cache
                        MessageType type;
                        if (json_get_type(json_payload, &type) == 0) {
                            if (type == MSG_ROOM_JOINED) {
                                int room_id;
                                if (json_get_int(json_payload, "room_id", &room_id) == 0) {
                                    state_cache_update_room(&dispatcher->state, room_id);
                                }
                            } else if (type == MSG_TIMER_UPDATE) {
                                int time_remaining;
                                if (json_get_int(json_payload, "time_remaining", &time_remaining) == 0) {
                                    state_cache_update_timer(&dispatcher->state, time_remaining);
                                }
                            }
                        }
                    }
                }
                
                free(msg.data);
            }
        }
        
        // Process messages from UDP -> forward to WebSocket
        if (queue_size(&dispatcher->from_udp_queue) > 0) {
            Message msg;
            if (queue_pop(&dispatcher->from_udp_queue, &msg)) {
                // Forward stroke data to WebSocket
                queue_push(&dispatcher->to_ws_queue, msg.data, msg.len, -1);  // broadcast
                free(msg.data);
            }
        }
        
        // Small sleep to prevent busy waiting
        usleep(1000);  // 1ms
    }
    
    printf("[DISPATCHER] Main loop ended\n");
    return NULL;
}
