#include "tcp_server.h"
#include "tcp_handler.h"
#include "../utils/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

#define MAX_CLIENTS 100

static int tcp_server_fd = -1;
static Player players[MAX_CLIENTS];
static int player_count = 0;
static pthread_t tcp_thread;
static volatile bool tcp_running = false;

void* tcp_server_thread(void* arg) {
    (void)arg;
    
    fd_set read_fds;
    struct timeval timeout;
    
    while (tcp_running) {
        FD_ZERO(&read_fds);
        FD_SET(tcp_server_fd, &read_fds);
        
        int max_fd = tcp_server_fd;
        
        // Add all player sockets
        for (int i = 0; i < player_count; i++) {
            if (players[i].fd > 0) {
                FD_SET(players[i].fd, &read_fds);
                if (players[i].fd > max_fd) {
                    max_fd = players[i].fd;
                }
            }
        }
        
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0 && errno != EINTR) {
            perror("select error");
            continue;
        }
        
        // Check for new connections
        if (FD_ISSET(tcp_server_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_fd = accept(tcp_server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                perror("accept failed");
                continue;
            }
            
            // Add new player
            if (player_count < MAX_CLIENTS) {
                Player* player = &players[player_count];
                memset(player, 0, sizeof(Player));
                player->fd = client_fd;
                player->recv_buffer_len = 0;
                inet_ntop(AF_INET, &client_addr.sin_addr, player->ip, INET_ADDRSTRLEN);
                player_count++;
                
                printf("[TCP] New connection from %s (fd=%d)\n", player->ip, client_fd);
            } else {
                printf("[TCP] Max clients reached, rejecting connection\n");
                close(client_fd);
            }
        }
        
        // Check for data from players
        for (int i = 0; i < player_count; i++) {
            if (players[i].fd > 0 && FD_ISSET(players[i].fd, &read_fds)) {
                // Read into the player's receive buffer after existing data
                int space_available = BUFFER_SIZE - players[i].recv_buffer_len;
                if (space_available <= 0) {
                    printf("[TCP] Buffer overflow for player %u, disconnecting\n", players[i].player_id);
                    handle_disconnect(&players[i]);
                    close(players[i].fd);
                    for (int j = i; j < player_count - 1; j++) {
                        players[j] = players[j + 1];
                    }
                    player_count--;
                    i--;
                    continue;
                }
                
                int bytes_read = recv(players[i].fd, 
                                    players[i].recv_buffer + players[i].recv_buffer_len,
                                    space_available, 0);
                
                if (bytes_read <= 0) {
                    // Client disconnected
                    printf("[TCP] Client %u disconnected (fd=%d)\n", players[i].player_id, players[i].fd);
                    handle_disconnect(&players[i]);
                    close(players[i].fd);
                    
                    // Remove from array
                    for (int j = i; j < player_count - 1; j++) {
                        players[j] = players[j + 1];
                    }
                    player_count--;
                    i--;
                } else {
                    players[i].recv_buffer_len += bytes_read;
                    
                    // Process all complete messages in buffer
                    int processed = 0;
                    while (processed < players[i].recv_buffer_len) {
                        // Need at least 4 bytes for length prefix
                        if (players[i].recv_buffer_len - processed < 4) break;
                        
                        // Read message length
                        uint32_t msg_len;
                        memcpy(&msg_len, players[i].recv_buffer + processed, 4);
                        msg_len = ntohl(msg_len);
                        
                        // Check if full message is available
                        if (players[i].recv_buffer_len - processed < 4 + (int)msg_len) break;
                        
                        // Process this complete message
                        handle_tcp_message(&players[i], 
                                         players[i].recv_buffer + processed, 
                                         4 + msg_len);
                        
                        processed += 4 + msg_len;
                    }
                    
                    // Move remaining incomplete data to start of buffer
                    if (processed > 0) {
                        int remaining = players[i].recv_buffer_len - processed;
                        if (remaining > 0) {
                            memmove(players[i].recv_buffer, 
                                   players[i].recv_buffer + processed, 
                                   remaining);
                        }
                        players[i].recv_buffer_len = remaining;
                    }
                }
            }
        }
    }
    
    return NULL;
}

int tcp_server_start(int port) {
    tcp_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_server_fd < 0) {
        perror("Failed to create TCP socket");
        return -1;
    }
    
    int opt = 1;
    setsockopt(tcp_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(tcp_server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Failed to bind TCP socket");
        close(tcp_server_fd);
        return -1;
    }
    
    if (listen(tcp_server_fd, 10) < 0) {
        perror("Failed to listen on TCP socket");
        close(tcp_server_fd);
        return -1;
    }
    
    memset(players, 0, sizeof(players));
    player_count = 0;
    tcp_running = true;
    
    if (pthread_create(&tcp_thread, NULL, tcp_server_thread, NULL) != 0) {
        perror("Failed to create TCP server thread");
        close(tcp_server_fd);
        return -1;
    }
    
    printf("[TCP] Server started on port %d\n", port);
    return 0;
}

void tcp_server_stop() {
    tcp_running = false;
    
    // Close all client connections
    for (int i = 0; i < player_count; i++) {
        if (players[i].fd > 0) {
            close(players[i].fd);
        }
    }
    
    if (tcp_server_fd >= 0) {
        close(tcp_server_fd);
        tcp_server_fd = -1;
    }
    
    pthread_join(tcp_thread, NULL);
    printf("[TCP] Server stopped\n");
}

void tcp_send_timer_updates(Room* room) {
    char timer_msg[128];
    snprintf(timer_msg, sizeof(timer_msg), 
             "{\"time_remaining\":%d}", room->time_remaining);
    broadcast_to_room(room, MSG_TIMER_UPDATE, timer_msg, NULL);
}

Player* find_player_by_ip(const char* ip) {
    for (int i = 0; i < player_count; i++) {
        if (players[i].fd > 0 && strcmp(players[i].ip, ip) == 0) {
            return &players[i];
        }
    }
    return NULL;
}
