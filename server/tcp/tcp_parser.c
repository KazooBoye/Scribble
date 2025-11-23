#include "tcp_parser.h"
#include "../utils/json.h"
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

int serialize_tcp_message(MessageType type, const char* json, char* buffer, int buffer_size) {
    (void)type;  // Type is encoded in JSON
    if (!json || !buffer) return -1;
    
    int json_len = strlen(json);
    int total_len = 4 + json_len;
    
    if (total_len > buffer_size) return -1;
    
    uint32_t len_network = htonl(json_len);
    memcpy(buffer, &len_network, 4);
    memcpy(buffer + 4, json, json_len);
    
    return total_len;
}

int deserialize_tcp_message(const char* buffer, int len, MessageType* type, char** json) {
    if (len < 4) return -1;
    
    uint32_t msg_len;
    memcpy(&msg_len, buffer, 4);
    msg_len = ntohl(msg_len);
    
    if (len < 4 + (int)msg_len) return -1;
    
    *json = malloc(msg_len + 1);
    if (!*json) return -1;
    
    memcpy(*json, buffer + 4, msg_len);
    (*json)[msg_len] = '\0';
    
    // Extract type from JSON
    json_get_type(*json, type);
    
    return 4 + msg_len;
}
