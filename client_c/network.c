/*
 * Scribble Game - Client Networking Library (C)
 * TCP/UDP socket implementation for client-server communication
 */

#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>

// Global state
static int tcp_fd = -1;
static struct sockaddr_in server_tcp_addr;
static pthread_t receive_thread;
static message_callback_t message_callback = NULL;
static bool connected = false;
static bool running = false;
static char last_error[256] = {0};

// Receive buffer for TCP message framing
#define BUFFER_SIZE 8192
static uint8_t recv_buffer[BUFFER_SIZE];
static size_t buffer_pos = 0;

// Set error message
static void set_error(const char* msg) {
    snprintf(last_error, sizeof(last_error), "%s", msg);
}

// TCP receive thread function
static void* tcp_receive_thread(void* arg) {
    (void)arg;
    
    while (running && connected) {
        uint8_t temp[4096];
        ssize_t bytes_received = recv(tcp_fd, temp, sizeof(temp), 0);
        
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                set_error("Server disconnected");
            } else {
                set_error("TCP receive error");
            }
            connected = false;
            break;
        }
        
        // Append to buffer
        if (buffer_pos + bytes_received > BUFFER_SIZE) {
            set_error("Receive buffer overflow");
            connected = false;
            break;
        }
        
        memcpy(recv_buffer + buffer_pos, temp, bytes_received);
        buffer_pos += bytes_received;
        
        // Parse length-prefixed messages
        while (buffer_pos >= 4) {
            // Read 4-byte length prefix (big-endian)
            uint32_t msg_length = ntohl(*(uint32_t*)recv_buffer);
            
            // Check if we have full message
            if (buffer_pos < 4 + msg_length) {
                break;  // Wait for more data
            }
            
            // Extract message
            char* json_str = (char*)malloc(msg_length + 1);
            if (!json_str) {
                set_error("Memory allocation failed");
                connected = false;
                break;
            }
            
            memcpy(json_str, recv_buffer + 4, msg_length);
            json_str[msg_length] = '\0';
            
            // Remove processed message from buffer
            buffer_pos -= (4 + msg_length);
            memmove(recv_buffer, recv_buffer + 4 + msg_length, buffer_pos);
            
            // Call callback if registered
            if (message_callback) {
                message_callback(json_str);
            }
            
            free(json_str);
        }
    }
    
    return NULL;
}

int network_connect(const char* host, int tcp_port) {
    // Resolve hostname
    struct hostent* server = gethostbyname(host);
    if (!server) {
        set_error("Failed to resolve hostname");
        return -1;
    }
    
    // Create TCP socket
    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_fd < 0) {
        set_error("Failed to create TCP socket");
        return -1;
    }
    
    // Set TCP_NODELAY for low latency
    int flag = 1;
    setsockopt(tcp_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    
    // Setup server address
    memset(&server_tcp_addr, 0, sizeof(server_tcp_addr));
    server_tcp_addr.sin_family = AF_INET;
    memcpy(&server_tcp_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_tcp_addr.sin_port = htons(tcp_port);
    
    // Connect TCP
    if (connect(tcp_fd, (struct sockaddr*)&server_tcp_addr, sizeof(server_tcp_addr)) < 0) {
        set_error("TCP connection failed");
        close(tcp_fd);
        tcp_fd = -1;
        return -1;
    }
    
    connected = true;
    running = true;
    buffer_pos = 0;
    
    // Start receive thread
    if (pthread_create(&receive_thread, NULL, tcp_receive_thread, NULL) != 0) {
        set_error("Failed to create receive thread");
        network_disconnect();
        return -1;
    }
    
    printf("[Client C] Connected to %s:%d (TCP)\n", host, tcp_port);
    return 0;
}

int network_send_tcp(const char* message_json) {
    if (!connected || tcp_fd < 0) {
        set_error("Not connected");
        return -1;
    }
    
    size_t json_len = strlen(message_json);
    
    // Prepare length prefix (4 bytes, big-endian)
    uint32_t length_prefix = htonl((uint32_t)json_len);
    
    // Send length prefix
    ssize_t sent = send(tcp_fd, &length_prefix, 4, 0);
    if (sent != 4) {
        set_error("TCP send failed (length prefix)");
        return -1;
    }
    
    // Send JSON data
    sent = send(tcp_fd, message_json, json_len, 0);
    if (sent != (ssize_t)json_len) {
        set_error("TCP send failed (data)");
        return -1;
    }
    
    return 0;
}

void network_set_callback(message_callback_t callback) {
    message_callback = callback;
}

int network_is_connected(void) {
    return connected ? 1 : 0;
}

void network_disconnect(void) {
    running = false;
    connected = false;
    
    // Close socket
    if (tcp_fd >= 0) {
        close(tcp_fd);
        tcp_fd = -1;
    }
    
    // Wait for receive thread to finish
    if (receive_thread) {
        pthread_join(receive_thread, NULL);
    }
    
    printf("[Client C] Disconnected\n");
}

const char* network_get_error(void) {
    return last_error;
}
