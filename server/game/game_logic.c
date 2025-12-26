#include "game_logic.h"
#include "../utils/logger.h"
#include "../utils/timer.h"
#include "../utils/json.h"
#include "../tcp/tcp_handler.h"
#include "stats.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static char** word_list = NULL;
static int word_count = 0;

int load_word_list(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("Failed to open word list");
        return -1;
    }
    
    // Count lines
    word_count = 0;
    char line[MAX_WORD_LEN];
    while (fgets(line, sizeof(line), f)) {
        word_count++;
    }
    
    rewind(f);
    
    word_list = malloc(sizeof(char*) * word_count);
    if (!word_list) {
        fclose(f);
        return -1;
    }
    
    int i = 0;
    while (fgets(line, sizeof(line), f) && i < word_count) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        word_list[i] = strdup(line);
        i++;
    }
    
    fclose(f);
    srand(time(NULL));
    
    printf("[GAME] Loaded %d words\n", word_count);
    return 0;
}

const char* get_random_word() {
    if (!word_list || word_count == 0) return "unknown";
    return word_list[rand() % word_count];
}

void init_room(Room* room, uint32_t room_id, bool is_private) {
    memset(room, 0, sizeof(Room));
    room->room_id = room_id;
    room->is_private = is_private;
    room->created_at = get_current_time_ms();
    room->state = ROOM_WAITING;
    room->current_drawer_idx = -1;
    room->total_rounds = 0;  // Will be set when game starts
    
    if (is_private) {
        // Generate random 6-character room code
        const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        for (int i = 0; i < 6; i++) {
            room->room_code[i] = chars[rand() % 36];
        }
        room->room_code[6] = '\0';
    }
    
    log_room_event(room_id, "created", is_private ? "private" : "public");
}

int add_player_to_room(Room* room, Player* player) {
    if (room->player_count >= MAX_PLAYERS) {
        return -1;
    }
    
    room->players[room->player_count] = player;
    room->player_count++;
    player->state = PLAYER_IN_ROOM;
    
    log_room_event(room->room_id, "player_joined", player->username);
    
    return 0;
}

int remove_player_from_room(Room* room, Player* player) {
    int player_idx = -1;
    
    // Find player index
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i] == player) {
            player_idx = i;
            break;
        }
    }
    
    if (player_idx == -1) return -1;
    
    bool was_drawing = player->is_drawing;
    bool was_in_game = (room->state == ROOM_PLAYING);
    
    // Shift remaining players
    for (int j = player_idx; j < room->player_count - 1; j++) {
        room->players[j] = room->players[j + 1];
    }
    room->players[room->player_count - 1] = NULL;
    room->player_count--;
    
    printf("[GAME] Player %s left. Remaining: %d players\n", player->username, room->player_count);
    
    // If game is in progress, adjust rounds and drawer index
    if (was_in_game && room->player_count >= 2) {
        // Recalculate total rounds based on remaining players who haven't drawn
        int remaining_rounds = 0;
        for (int i = 0; i < room->player_count; i++) {
            if (room->players[i] && !room->players[i]->has_drawn) {
                remaining_rounds++;
            }
        }
        
        room->total_rounds = room->round_number + remaining_rounds;
        printf("[GAME] Adjusted total rounds to %d (current: %d, remaining: %d)\n",
               room->total_rounds, room->round_number, remaining_rounds);
        
        // If current drawer left, adjust drawer index
        if (was_drawing) {
            // Move drawer index back one since we shifted array
            if (room->current_drawer_idx > 0) {
                room->current_drawer_idx--;
            } else {
                room->current_drawer_idx = room->player_count - 1;
            }
            printf("[GAME] Current drawer left, ending round early\n");
            end_round(room);
        } else {
            // Adjust drawer index if it was after the removed player
            if (room->current_drawer_idx > player_idx) {
                room->current_drawer_idx--;
            }
        }
    } else if (room->player_count < 2 && was_in_game) {
        // Not enough players to continue
        printf("[GAME] Not enough players remaining, ending game\n");
        end_game(room);
    }
    
    log_room_event(room->room_id, "player_left", player->username);
    return 0;
}

void start_game(Room* room) {
    if (room->player_count < 2) return;
    
    room->state = ROOM_PLAYING;
    room->round_number = 0;
    room->total_rounds = room->player_count;  // Set total rounds based on current player count
    
    // Reset all player scores and stats tracking
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i]) {
            room->players[i]->score = 0;
            room->players[i]->state = PLAYER_PLAYING;
            room->players[i]->has_drawn = false;  // Track if player has had their turn
            room->players[i]->correct_guesses_this_game = 0;
            room->players[i]->rounds_drawn_this_game = 0;
        }
    }
    
    printf("[GAME] Starting game with %d players, %d total rounds\n", 
           room->player_count, room->total_rounds);
    log_room_event(room->room_id, "game_started", "");
    start_next_round(room);
}

void start_next_round(Room* room) {
    room->round_number++;
    
    // Check if all remaining players have had their turn or if we've exceeded total rounds
    int players_who_havent_drawn = 0;
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i] && !room->players[i]->has_drawn) {
            players_who_havent_drawn++;
        }
    }
    
    if (players_who_havent_drawn == 0 || room->round_number > room->total_rounds) {
        printf("[GAME] All players have drawn or rounds completed. Ending game.\n");
        end_game(room);
        return;
    }
    
    // Find next drawer who hasn't drawn yet
    int attempts = 0;
    do {
        room->current_drawer_idx = (room->current_drawer_idx + 1) % room->player_count;
        attempts++;
        
        // Prevent infinite loop
        if (attempts > room->player_count) {
            printf("[GAME] ERROR: Could not find next drawer\n");
            end_game(room);
            return;
        }
    } while (room->players[room->current_drawer_idx] == NULL || 
             room->players[room->current_drawer_idx]->has_drawn);
    
    // Mark this player as having drawn
    room->players[room->current_drawer_idx]->has_drawn = true;
    room->players[room->current_drawer_idx]->rounds_drawn_this_game++;
    
    printf("[GAME] Round %d/%d - Player %d (%s) is drawing\n", 
           room->round_number, room->total_rounds, room->current_drawer_idx,
           room->players[room->current_drawer_idx]->username);
    
    // Select random word
    const char* word = get_random_word();
    strncpy(room->current_word, word, MAX_WORD_LEN - 1);
    room->current_word[MAX_WORD_LEN - 1] = '\0';
    
    room->time_remaining = ROUND_TIME;
    room->round_start_time = get_current_time_ms();
    room->stroke_count = 0;
    
    // Reset player states and set round start time for guess tracking
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i]) {
            room->players[i]->is_drawing = (i == room->current_drawer_idx);
            room->players[i]->has_guessed = false;
            room->players[i]->state = room->players[i]->is_drawing ? 
                                      PLAYER_DRAWING : PLAYER_GUESSING;
            room->players[i]->round_start_time = room->round_start_time;
            printf("[GAME] Player %u (%s) - is_drawing: %d, state: %d\n",
                   room->players[i]->player_id, room->players[i]->username,
                   room->players[i]->is_drawing, room->players[i]->state);
        }
    }
    
    log_room_event(room->room_id, "round_started", room->current_word);
}

void end_round(Room* room) {
    log_room_event(room->room_id, "round_ended", "");
    
    // Broadcast round end message
    char* room_state = json_create_room_state(room);
    broadcast_to_room(room, MSG_ROUND_END, room_state, NULL);
    free(room_state);
    
    start_next_round(room);
    
    // Broadcast new round start
    room_state = json_create_room_state(room);
    broadcast_to_room(room, MSG_ROUND_START, room_state, NULL);
    free(room_state);
    
    // Send word to new drawer
    if (room->players[room->current_drawer_idx]) {
        char word_msg[256];
        snprintf(word_msg, sizeof(word_msg), 
                 "{\"word\":\"%s\"}", room->current_word);
        printf("[GAME] Sending MSG_WORD_TO_DRAW to player %u (%s) - word: %s\n",
               room->players[room->current_drawer_idx]->player_id,
               room->players[room->current_drawer_idx]->username,
               room->current_word);
        send_tcp_message(room->players[room->current_drawer_idx]->fd, 
                       MSG_WORD_TO_DRAW, word_msg);
    } else {
        printf("[GAME] ERROR: current_drawer_idx=%d but player is NULL!\n", 
               room->current_drawer_idx);
    }
}

void end_game(Room* room) {
    room->state = ROOM_ENDED;
    
    // Find winner
    int max_score = -1;
    Player* winner = NULL;
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i] && room->players[i]->score > max_score) {
            max_score = room->players[i]->score;
            winner = room->players[i];
        }
    }
    
    // Update stats for all players
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i]) {
            Player* p = room->players[i];
            bool won = (p == winner);
            update_game_stats(p, won, p->correct_guesses_this_game, p->rounds_drawn_this_game);
        }
    }
    
    if (winner) {
        log_room_event(room->room_id, "game_ended", winner->username);
    }
    
    // Broadcast game end with final scores
    char* room_state = json_create_room_state(room);
    broadcast_to_room(room, MSG_GAME_END, room_state, NULL);
    free(room_state);
    
    printf("[GAME] Game ended for room %u. Winner: %s with %d points\n",
           room->room_id, winner ? winner->username : "none", max_score);
}

int process_guess(Room* room, Player* player, const char* guess) {
    if (player->is_drawing || player->has_guessed) {
        return 0;  // Ignore
    }
    
    // Case-insensitive comparison
    char guess_lower[MAX_CHAT_LEN];
    char word_lower[MAX_WORD_LEN];
    
    strncpy(guess_lower, guess, MAX_CHAT_LEN - 1);
    strncpy(word_lower, room->current_word, MAX_WORD_LEN - 1);
    
    for (int i = 0; guess_lower[i]; i++) {
        if (guess_lower[i] >= 'A' && guess_lower[i] <= 'Z') {
            guess_lower[i] += 32;
        }
    }
    
    for (int i = 0; word_lower[i]; i++) {
        if (word_lower[i] >= 'A' && word_lower[i] <= 'Z') {
            word_lower[i] += 32;
        }
    }
    
    if (strcmp(guess_lower, word_lower) == 0) {
        // Correct guess!
        player->has_guessed = true;
        player->correct_guesses_this_game++;
        
        // Calculate guess time for stats
        uint64_t guess_time = get_current_time_ms() - player->round_start_time;
        update_fastest_guess(player, guess_time);
        
        // Score based on time remaining (more time = more points)
        int points = 10 + (room->time_remaining * 90 / ROUND_TIME);
        player->score += points;
        
        log_guess(room->room_id, player->player_id, guess, true);
        log_score(room->room_id, player->player_id, player->score);
        
        // Broadcast score update
        char* room_state = json_create_room_state(room);
        broadcast_to_room(room, MSG_SCORE_UPDATE, room_state, NULL);
        free(room_state);
        
        // Check if all players have guessed
        int guessed_count = 0;
        for (int i = 0; i < room->player_count; i++) {
            if (room->players[i] && 
                (room->players[i]->has_guessed || room->players[i]->is_drawing)) {
                guessed_count++;
            }
        }
        
        if (guessed_count == room->player_count) {
            // All guessed, end round early
            end_round(room);
        }
        
        return 1;  // Correct
    }
    
    log_guess(room->room_id, player->player_id, guess, false);
    return 0;  // Wrong
}

void update_timer(Room* room) {
    if (room->state != ROOM_PLAYING) return;
    
    uint64_t elapsed = (get_current_time_ms() - room->round_start_time) / 1000;
    room->time_remaining = ROUND_TIME - (int)elapsed;
    
    if (room->time_remaining <= 0) {
        room->time_remaining = 0;
        end_round(room);
    }
    
    log_timer(room->room_id, room->time_remaining);
}

void check_game_start_countdown(Room* room) {
    if (!room->countdown_active || room->state != ROOM_WAITING) return;
    
    uint64_t elapsed = (get_current_time_ms() - room->game_start_countdown) / 1000;
    int remaining = 15 - (int)elapsed;
    
    // Broadcast countdown update to players
    if (remaining > 0 && remaining <= 15) {
        char countdown_msg[128];
        snprintf(countdown_msg, sizeof(countdown_msg), "{\"countdown\":%d}", remaining);
        broadcast_to_room(room, MSG_COUNTDOWN_UPDATE, countdown_msg, NULL);
    }
    
    // Start game after 15 seconds if at least 2 players and round hasn't started
    if (elapsed >= 15 && room->player_count >= 2) {
        printf("[COUNTDOWN] Starting game for room %u after %llu seconds with %d players\n", 
               room->room_id, elapsed, room->player_count);
        room->countdown_active = false;
        start_game(room);
        
        // Notify all players with MSG_GAME_START
        char* game_state = json_create_room_state(room);
        broadcast_to_room(room, MSG_GAME_START, game_state, NULL);
        free(game_state);
        
        // Send the actual word to the drawer
        if (room->players[room->current_drawer_idx]) {
            char word_msg[256];
            snprintf(word_msg, sizeof(word_msg), 
                     "{\"word\":\"%s\"}", room->current_word);
            printf("[GAME] COUNTDOWN COMPLETE - Sending MSG_WORD_TO_DRAW to player %u (%s) - word: %s\n",
                   room->players[room->current_drawer_idx]->player_id,
                   room->players[room->current_drawer_idx]->username,
                   room->current_word);
            send_tcp_message(room->players[room->current_drawer_idx]->fd, 
                           MSG_WORD_TO_DRAW, word_msg);
        } else {
            printf("[GAME] COUNTDOWN COMPLETE ERROR: current_drawer_idx=%d but player is NULL!\n", 
                   room->current_drawer_idx);
        }
        
        log_room_event(room->room_id, "game_started_countdown", "");
    }
}

void add_stroke(Room* room, const Stroke* stroke) {
    if (room->stroke_count < MAX_STROKES) {
        room->strokes[room->stroke_count] = *stroke;
        room->strokes[room->stroke_count].stroke_id = room->stroke_count;
        log_stroke(room->room_id, room->stroke_count, stroke);
        room->stroke_count++;
    }
}

void cleanup_word_list() {
    if (word_list) {
        for (int i = 0; i < word_count; i++) {
            free(word_list[i]);
        }
        free(word_list);
        word_list = NULL;
    }
}
