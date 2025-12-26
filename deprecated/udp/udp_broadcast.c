#include "udp_broadcast.h"
#include "../game/matchmaking.h"
#include "../game/game_logic.h"
#include "../utils/logger.h"
#include "../utils/endian_compat.h"
#include <string.h>
#include <arpa/inet.h>

int serialize_udp_stroke(const Stroke* stroke, uint32_t room_id, char* buffer, int buffer_size) {
    if (buffer_size < sizeof(UDPMessage)) return -1;
    
    UDPMessage* msg = (UDPMessage*)buffer;
    msg->type = UDP_STROKE;
    msg->room_id = htonl(room_id);
    msg->data.stroke = *stroke;
    
    // Convert to network byte order
    msg->data.stroke.stroke_id = htonl(stroke->stroke_id);
    msg->data.stroke.color = htonl(stroke->color);
    msg->data.stroke.timestamp = htobe64(stroke->timestamp);
    
    return sizeof(UDPMessage);
}

int deserialize_udp_stroke(const char* buffer, int len, Stroke* stroke, uint32_t* room_id) {
    if (len < sizeof(UDPMessage)) return -1;
    
    UDPMessage* msg = (UDPMessage*)buffer;
    
    if (msg->type != UDP_STROKE) return -1;
    
    *room_id = ntohl(msg->room_id);
    *stroke = msg->data.stroke;
    
    // Convert from network byte order
    stroke->stroke_id = ntohl(stroke->stroke_id);
    stroke->color = ntohl(stroke->color);
    stroke->timestamp = be64toh(stroke->timestamp);
    
    return 0;
}

void broadcast_stroke_to_room(int udp_fd, Room* room, const Stroke* stroke, 
                               struct sockaddr_in* exclude_addr) {
    char buffer[BUFFER_SIZE];
    int len = serialize_udp_stroke(stroke, room->room_id, buffer, sizeof(buffer));
    
    if (len < 0) return;
    
    // Send to all players in room
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i] && room->players[i]->fd > 0) {
            // Get player's UDP address (we'll use TCP fd as reference)
            // In real implementation, we'd track UDP addresses separately
            struct sockaddr_in player_addr;
            memset(&player_addr, 0, sizeof(player_addr));
            player_addr.sin_family = AF_INET;
            inet_pton(AF_INET, room->players[i]->ip, &player_addr.sin_addr);
            player_addr.sin_port = htons(UDP_PORT + 1);  // Client UDP port
            
            // Skip sender
            if (exclude_addr && 
                player_addr.sin_addr.s_addr == exclude_addr->sin_addr.s_addr &&
                player_addr.sin_port == exclude_addr->sin_port) {
                continue;
            }
            
            sendto(udp_fd, buffer, len, 0, 
                   (struct sockaddr*)&player_addr, sizeof(player_addr));
        }
    }
}
