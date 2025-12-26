#ifndef UDP_BROADCAST_H
#define UDP_BROADCAST_H

#include "../protocol.h"
#include <sys/socket.h>
#include <netinet/in.h>

int serialize_udp_stroke(const Stroke* stroke, uint32_t room_id, char* buffer, int buffer_size);
int deserialize_udp_stroke(const char* buffer, int len, Stroke* stroke, uint32_t* room_id);
void broadcast_stroke_to_room(int udp_fd, Room* room, const Stroke* stroke, 
                               struct sockaddr_in* exclude_addr);

#endif // UDP_BROADCAST_H
