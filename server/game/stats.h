#ifndef STATS_H
#define STATS_H

#include "../protocol.h"

// Player statistics structure
typedef struct {
    char username[MAX_USERNAME];
    uint32_t games_played;
    uint32_t games_won;
    uint64_t total_score;
    uint32_t total_correct_guesses;
    uint32_t total_rounds_drawn;
    uint64_t fastest_guess_ms;  // Best guess time in milliseconds
    uint64_t last_played;       // Timestamp of last game
} PlayerStats;

// Initialize stats system
void init_stats_system();

// Load player stats from file (returns 0 if found, -1 if new player)
int load_player_stats(const char* username, PlayerStats* stats);

// Save player stats to file
void save_player_stats(const PlayerStats* stats);

// Update stats after game end
void update_game_stats(Player* player, bool won, int correct_guesses, int rounds_drawn);

// Update fastest guess time
void update_fastest_guess(Player* player, uint64_t guess_time_ms);

// Get leaderboard (top N players by total score)
int get_leaderboard(PlayerStats* leaderboard, int max_count);

#endif // STATS_H
