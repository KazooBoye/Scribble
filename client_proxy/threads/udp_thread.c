#include "udp_thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

int udp_thread_init(UDPThread* udp, Dispatcher* dispatcher, const char* host, int port) {
    udp->dispatcher = dispatcher;
    udp->server_port = port;
    strncpy(udp->server_host, host, sizeof(udp->server_host) - 1);
    udp->running = true;
    
    if (pthread_create(&udp->thread, NULL, udp_thread_func, udp) != 0) {
        perror("Failed to create UDP thread");
        return -1;
    }
    
    printf("[UDP] Thread started, targeting %s:%d\n", host, port);
    return 0;
}

void udp_thread_destroy(UDPThread* udp) {
    udp->running = false;
    pthread_join(udp->thread, NULL);
    printf("[UDP] Thread stopped\n");
}

void* udp_thread_func(void* arg) {
    UDPThread* udp = (UDPThread*)arg;
    
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("Failed to create UDP socket");
        return NULL;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(udp->server_port);
    inet_pton(AF_INET, udp->server_host, &server_addr.sin_addr);
    
    // Bind to local port for receiving
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(UDP_PORT + 1);  // Client UDP port
    
    if (bind(sock_fd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("Failed to bind UDP socket");
        close(sock_fd);
        return NULL;
    }
    
    printf("[UDP] Socket bound and ready\n");
    
    while (udp->running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock_fd, &read_fds);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;  // 10ms for low latency
        
        int activity = select(sock_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0 && errno != EINTR) {
            perror("UDP select error");
            break;
        }
        
        // Receive from server
        if (FD_ISSET(sock_fd, &read_fds)) {
            char buffer[BUFFER_SIZE];
            struct sockaddr_in from_addr;
            socklen_t from_len = sizeof(from_addr);
            
            int bytes = recvfrom(sock_fd, buffer, sizeof(buffer), 0,
                               (struct sockaddr*)&from_addr, &from_len);
            
            if (bytes > 0) {
                // Push to dispatcher
                queue_push(&udp->dispatcher->from_udp_queue, buffer, bytes, -1);
            }
        }
        
        // Send to server (from dispatcher)
        if (queue_size(&udp->dispatcher->to_udp_queue) > 0) {
            Message msg;
            if (queue_pop(&udp->dispatcher->to_udp_queue, &msg)) {
                sendto(sock_fd, msg.data, msg.len, 0,
                      (struct sockaddr*)&server_addr, sizeof(server_addr));
                free(msg.data);
            }
        }
    }
    
    close(sock_fd);
    return NULL;
}
