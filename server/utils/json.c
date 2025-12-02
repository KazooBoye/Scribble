#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple JSON builder - not a full parser, just for creating messages
char* json_create_message(MessageType type, const char* data) {
    char* buffer = malloc(BUFFER_SIZE);
    if (!buffer) return NULL;
    
    snprintf(buffer, BUFFER_SIZE, "{\"type\":%d,\"data\":%s}", type, data);
    return buffer;
}

char* json_create_simple(MessageType type, const char* key, const char* value) {
    char* buffer = malloc(BUFFER_SIZE);
    if (!buffer) return NULL;
    
    snprintf(buffer, BUFFER_SIZE, "{\"type\":%d,\"data\":{\"%s\":\"%s\"}}", 
             type, key, value);
    return buffer;
}

char* json_create_error(const char* error_msg) {
    char* buffer = malloc(BUFFER_SIZE);
    if (!buffer) return NULL;
    
    snprintf(buffer, BUFFER_SIZE, 
             "{\"type\":%d,\"data\":{\"error\":\"%s\"}}", 
             MSG_ERROR, error_msg);
    return buffer;
}

char* json_create_player_info(const Player* player) {
    char* buffer = malloc(BUFFER_SIZE);
    if (!buffer) return NULL;
    
    bool is_online = (player->state != PLAYER_DISCONNECTED);
    snprintf(buffer, BUFFER_SIZE,
             "{\"player_id\":%u,\"username\":\"%s\",\"score\":%d,\"is_drawing\":%s,\"online\":%s}",
             player->player_id, player->username, player->score,
             player->is_drawing ? "true" : "false",
             is_online ? "true" : "false");
    return buffer;
}

char* json_create_room_state(const Room* room) {
    char* buffer = malloc(BUFFER_SIZE * 2);
    if (!buffer) return NULL;
    
    char players_json[BUFFER_SIZE] = "[";
    int added_count = 0;
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i]) {
            char* player_str = json_create_player_info(room->players[i]);
            if (player_str) {
                if (added_count > 0) strcat(players_json, ",");
                strcat(players_json, player_str);
                free(player_str);
                added_count++;
            }
        }
    }
    strcat(players_json, "]");
    
    // Word mask (hide some letters)
    char word_mask[MAX_WORD_LEN];
    int word_len = strlen(room->current_word);
    for (int i = 0; i < word_len; i++) {
        if (room->current_word[i] == ' ') {
            word_mask[i] = ' ';
        } else if (i == 0 || i == word_len - 1) {
            word_mask[i] = room->current_word[i];
        } else {
            word_mask[i] = '_';
        }
    }
    word_mask[word_len] = '\0';
    
    snprintf(buffer, BUFFER_SIZE * 2,
             "{\"room_id\":%u,\"room_code\":\"%s\",\"player_count\":%d,\"state\":%d,"
             "\"current_drawer\":%d,\"word_mask\":\"%s\",\"round\":%d,\"total_rounds\":%d,\"time_remaining\":%d,"
             "\"players\":%s}",
             room->room_id, room->room_code, room->player_count, room->state,
             room->current_drawer_idx, word_mask, room->round_number, room->total_rounds, room->time_remaining,
             players_json);
    
    return buffer;
}

// Simple JSON value extraction (not a full parser)
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

// Extract string from nested data field
int json_get_data_string(const char* json, const char* key, char* out, int out_size) {
    // Find "data":{ ... }
    const char* data_start = strstr(json, "\"data\":");
    if (!data_start) {
        // No data field, try direct extraction
        return json_get_string(json, key, out, out_size);
    }
    
    // Extract from data portion
    return json_get_string(data_start, key, out, out_size);
}

// Extract int from nested data field
int json_get_data_int(const char* json, const char* key, int* out) {
    // Find "data":{ ... }
    const char* data_start = strstr(json, "\"data\":");
    if (!data_start) {
        // No data field, try direct extraction
        return json_get_int(json, key, out);
    }
    
    // Extract from data portion
    return json_get_int(data_start, key, out);
}
