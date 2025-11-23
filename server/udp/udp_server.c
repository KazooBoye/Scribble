#include "udp_server.h"
#include "udp_broadcast.h"
#include "../game/matchmaking.h"
#include "../game/game_logic.h"
#include "../utils/logger.h"
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
    
    while (udp_running) {
        int bytes_read = recvfrom(udp_server_fd, buffer, sizeof(buffer), 0,
                                  (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes_read < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            perror("UDP recvfrom failed");
            continue;
        }
        
        Stroke stroke;
        uint32_t room_id;
        
        if (deserialize_udp_stroke(buffer, bytes_read, &stroke, &room_id) == 0) {
            Room* room = find_room_by_id(room_id);
            if (room) {
                // Add stroke to room
                add_stroke(room, &stroke);
                
                // Broadcast to all players except sender
                broadcast_stroke_to_room(udp_server_fd, room, &stroke, &client_addr);
                
                // Log stroke
                log_stroke(room_id, stroke.stroke_id, &stroke);
            }
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
