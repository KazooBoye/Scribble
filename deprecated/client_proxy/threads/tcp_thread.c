#include "tcp_thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

// Serialize TCP message with length prefix
int serialize_message(const char* json, char* buffer, int buffer_size) {
    int json_len = strlen(json);
    if (json_len + 4 > buffer_size) return -1;
    
    uint32_t len_network = htonl(json_len);
    memcpy(buffer, &len_network, 4);
    memcpy(buffer + 4, json, json_len);
    
    return 4 + json_len;
}

int tcp_thread_init(TCPThread* tcp, Dispatcher* dispatcher, const char* host, int port) {
    tcp->dispatcher = dispatcher;
    tcp->server_port = port;
    strncpy(tcp->server_host, host, sizeof(tcp->server_host) - 1);
    tcp->running = true;
    
    if (pthread_create(&tcp->thread, NULL, tcp_thread_func, tcp) != 0) {
        perror("Failed to create TCP thread");
        return -1;
    }
    
    printf("[TCP] Thread started, connecting to %s:%d\n", host, port);
    return 0;
}

void tcp_thread_destroy(TCPThread* tcp) {
    tcp->running = false;
    pthread_join(tcp->thread, NULL);
    printf("[TCP] Thread stopped\n");
}

void* tcp_thread_func(void* arg) {
    TCPThread* tcp = (TCPThread*)arg;
    
    // Connect to game server
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Failed to create TCP socket");
        return NULL;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(tcp->server_port);
    inet_pton(AF_INET, tcp->server_host, &server_addr.sin_addr);
    
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to game server");
        close(sock_fd);
        return NULL;
    }
    
    printf("[TCP] Connected to game server\n");
    
    while (tcp->running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock_fd, &read_fds);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;  // 100ms
        
        int activity = select(sock_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0 && errno != EINTR) {
            perror("TCP select error");
            break;
        }
        
        // Receive from server
        if (FD_ISSET(sock_fd, &read_fds)) {
            char buffer[BUFFER_SIZE];
            int bytes = recv(sock_fd, buffer, sizeof(buffer), 0);
            
            if (bytes <= 0) {
                printf("[TCP] Connection closed by server\n");
                break;
            }
            
            printf("[TCP] Received %d bytes from server\n", bytes);
            
            // Push to dispatcher (forward raw bytes, dispatcher will handle parsing)
            queue_push(&tcp->dispatcher->from_tcp_queue, buffer, bytes, -1);
        }
        
        // Send to server (from dispatcher)
        if (queue_size(&tcp->dispatcher->to_tcp_queue) > 0) {
            Message msg;
            if (queue_pop(&tcp->dispatcher->to_tcp_queue, &msg)) {
                // Serialize with length prefix
                char send_buffer[BUFFER_SIZE];
                msg.data[msg.len] = '\0';  // Ensure null-terminated
                int serialized_len = serialize_message(msg.data, send_buffer, sizeof(send_buffer));
                
                if (serialized_len > 0) {
                    printf("[TCP] Sending to server: %.*s\n", msg.len, msg.data);
                    int sent = send(sock_fd, send_buffer, serialized_len, 0);
                    if (sent < 0) {
                        perror("[TCP] Send failed");
                    } else {
                        printf("[TCP] Sent %d bytes\n", sent);
                    }
                }
                
                free(msg.data);
            }
        }
    }
    
    close(sock_fd);
    return NULL;
}
