#include "udp_server.h"
#include "udp_broadcast.h"
#include "../game/matchmaking.h"
#include "../game/game_logic.h"
#include "../tcp/tcp_server.h"
#include "../tcp/tcp_handler.h"
#include "../utils/logger.h"
#include "../utils/json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

static int udp_server_fd = -1;
static pthread_t udp_thread;
static volatile bool udp_running = false;

void* udp_server_thread(void* arg) {
    (void)arg;
    
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    printf("[UDP] Thread started, listening for packets...\n");
    
    while (udp_running) {
        int bytes_read = recvfrom(udp_server_fd, buffer, sizeof(buffer), 0,
                                  (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes_read < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            perror("UDP recvfrom failed");
            continue;
        }
        
        printf("[UDP] Received %d bytes from %s:%d\n", 
               bytes_read, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        Stroke stroke;
        uint32_t room_id;
        
        if (deserialize_udp_stroke(buffer, bytes_read, &stroke, &room_id) == 0) {
            printf("[UDP] Deserialized stroke successfully, room_id=%u\n", room_id);
            
            // Find the player by IP address
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            Player* sender = find_player_by_ip(client_ip);
            
            if (!sender) {
                printf("[UDP] Could not find player with IP %s\n", client_ip);
                continue;
            }
            
            // Get the sender's room
            Room* room = get_player_room(sender);
            if (!room) {
                printf("[UDP] Player %u (%s) not in a room\n", sender->player_id, sender->username);
                continue;
            }
            
            printf("[UDP] Player %u (%s) sent stroke to room %u\n", 
                   sender->player_id, sender->username, room->room_id);
            
            // Assign stroke ID and add to room
            stroke.stroke_id = room->stroke_count;
            add_stroke(room, &stroke);
            
            // Build JSON for TCP broadcast
            char stroke_json[512];
            snprintf(stroke_json, sizeof(stroke_json),
                     "{\"stroke_id\":%u,\"x1\":%.2f,\"y1\":%.2f,\"x2\":%.2f,\"y2\":%.2f,"
                     "\"color\":%u,\"thickness\":%u,\"timestamp\":%llu}",
                     stroke.stroke_id, stroke.x1, stroke.y1, stroke.x2, stroke.y2,
                     stroke.color, stroke.thickness, (unsigned long long)stroke.timestamp);
            
            // Broadcast to all players in room via TCP (excluding sender)
            printf("[UDP] Broadcasting stroke %u to room %u via TCP\n", 
                   stroke.stroke_id, room->room_id);
            broadcast_to_room(room, (MessageType)UDP_STROKE, stroke_json, sender);
            
            // Log stroke
            log_stroke(room->room_id, stroke.stroke_id, &stroke);
        } else {
            printf("[UDP] Failed to deserialize stroke\n");
        }
    }
    
    return NULL;
}

int udp_server_start(int port) {
    udp_server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_server_fd < 0) {
        perror("Failed to create UDP socket");
        return -1;
    }
    
    int opt = 1;
    setsockopt(udp_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(udp_server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Failed to bind UDP socket");
        close(udp_server_fd);
        return -1;
    }
    
    udp_running = true;
    
    if (pthread_create(&udp_thread, NULL, udp_server_thread, NULL) != 0) {
        perror("Failed to create UDP server thread");
        close(udp_server_fd);
        return -1;
    }
    
    printf("[UDP] Server started on port %d\n", port);
    return 0;
}

void udp_server_stop() {
    udp_running = false;
    if (udp_server_fd >= 0) {
        close(udp_server_fd);
        udp_server_fd = -1;
    }
    pthread_join(udp_thread, NULL);
    printf("[UDP] Server stopped\n");
}
