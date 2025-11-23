#ifndef LOGGER_H
#define LOGGER_H

#include "../protocol.h"

int logger_init(const char* filename);
void logger_close();
void log_event(const char* event_type, const char* format, ...);
void log_room_event(uint32_t room_id, const char* event, const char* details);
void log_player_event(uint32_t player_id, const char* event, const char* details);
void log_stroke(uint32_t room_id, uint32_t stroke_id, const Stroke* stroke);
void log_guess(uint32_t room_id, uint32_t player_id, const char* guess, bool correct);
void log_timer(uint32_t room_id, int time_remaining);
void log_score(uint32_t room_id, uint32_t player_id, int score);
void log_disconnect(uint32_t player_id, const char* reason);
void log_reconnect(uint32_t player_id, const char* session_token, bool success);

#endif // LOGGER_H
