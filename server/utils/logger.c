#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>

static FILE* log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int logger_init(const char* filename) {
    pthread_mutex_lock(&log_mutex);
    if (log_file != NULL) {
        fclose(log_file);
    }
    log_file = fopen(filename, "a");
    pthread_mutex_unlock(&log_mutex);
    
    if (log_file == NULL) {
        perror("Failed to open log file");
        return -1;
    }
    return 0;
}

void logger_close() {
    pthread_mutex_lock(&log_mutex);
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
    pthread_mutex_unlock(&log_mutex);
}

void log_event(const char* event_type, const char* format, ...) {
    if (log_file == NULL) return;
    
    pthread_mutex_lock(&log_mutex);
    
    time_t now;
    time(&now);
    struct tm* tm_info = localtime(&now);
    
    char time_buffer[64];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%dT%H:%M:%S", tm_info);
    
    fprintf(log_file, "{\"timestamp\":\"%s\",\"type\":\"%s\",\"data\":{", time_buffer, event_type);
    
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    
    fprintf(log_file, "}}\n");
    fflush(log_file);
    
    pthread_mutex_unlock(&log_mutex);
}

void log_room_event(uint32_t room_id, const char* event, const char* details) {
    log_event("room", "\"room_id\":%u,\"event\":\"%s\",\"details\":\"%s\"", 
              room_id, event, details);
}

void log_player_event(uint32_t player_id, const char* event, const char* details) {
    log_event("player", "\"player_id\":%u,\"event\":\"%s\",\"details\":\"%s\"", 
              player_id, event, details);
}

void log_stroke(uint32_t room_id, uint32_t stroke_id, const Stroke* stroke) {
    log_event("stroke", 
              "\"room_id\":%u,\"stroke_id\":%u,\"x1\":%.2f,\"y1\":%.2f,\"x2\":%.2f,\"y2\":%.2f,\"color\":%u,\"thickness\":%u",
              room_id, stroke_id, stroke->x1, stroke->y1, stroke->x2, stroke->y2, 
              stroke->color, stroke->thickness);
}

void log_guess(uint32_t room_id, uint32_t player_id, const char* guess, bool correct) {
    log_event("guess", "\"room_id\":%u,\"player_id\":%u,\"guess\":\"%s\",\"correct\":%s",
              room_id, player_id, guess, correct ? "true" : "false");
}

void log_timer(uint32_t room_id, int time_remaining) {
    log_event("timer", "\"room_id\":%u,\"time_remaining\":%d", room_id, time_remaining);
}

void log_score(uint32_t room_id, uint32_t player_id, int score) {
    log_event("score", "\"room_id\":%u,\"player_id\":%u,\"score\":%d", 
              room_id, player_id, score);
}

void log_disconnect(uint32_t player_id, const char* reason) {
    log_event("disconnect", "\"player_id\":%u,\"reason\":\"%s\"", player_id, reason);
}

void log_reconnect(uint32_t player_id, const char* session_token, bool success) {
    log_event("reconnect", "\"player_id\":%u,\"token\":\"%s\",\"success\":%s",
              player_id, session_token, success ? "true" : "false");
}
