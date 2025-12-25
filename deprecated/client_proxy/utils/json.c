#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple JSON helpers (same as server)
char* json_create_message(MessageType type, const char* data) {
    char* buffer = malloc(BUFFER_SIZE);
    if (!buffer) return NULL;
    
    snprintf(buffer, BUFFER_SIZE, "{\"type\":%d,\"data\":%s}", type, data);
    return buffer;
}

int json_get_string(const char* json, const char* key, char* out, int out_size) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":\"", key);
    
    const char* start = strstr(json, search);
    if (!start) return -1;
    
    start += strlen(search);
    const char* end = strchr(start, '"');
    if (!end) return -1;
    
    int len = end - start;
    if (len >= out_size) len = out_size - 1;
    
    strncpy(out, start, len);
    out[len] = '\0';
    
    return 0;
}

int json_get_int(const char* json, const char* key, int* out) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":", key);
    
    const char* start = strstr(json, search);
    if (!start) return -1;
    
    start += strlen(search);
    *out = atoi(start);
    
    return 0;
}

int json_get_type(const char* json, MessageType* type) {
    int t;
    if (json_get_int(json, "type", &t) == 0) {
        *type = (MessageType)t;
        return 0;
    }
    return -1;
}
