#include "stats.h"
#include "../utils/logger.h"
#include "../utils/timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define STATS_FILE "player_stats.txt"
#define TEMP_STATS_FILE "player_stats.tmp"

static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_stats_system() {
    // Create stats file if it doesn't exist
    FILE* f = fopen(STATS_FILE, "a");
    if (f) {
        fclose(f);
        printf("[STATS] Stats system initialized\n");
    } else {
        fprintf(stderr, "[STATS] Warning: Could not create stats file\n");
    }
}

int load_player_stats(const char* username, PlayerStats* stats) {
    pthread_mutex_lock(&stats_mutex);
    
    FILE* f = fopen(STATS_FILE, "r");
    if (!f) {
        pthread_mutex_unlock(&stats_mutex);
        
        // Initialize new player stats
        memset(stats, 0, sizeof(PlayerStats));
        strncpy(stats->username, username, MAX_USERNAME - 1);
        stats->fastest_guess_ms = UINT64_MAX;  // No record yet
        return -1;  // New player
    }
    
    char line[512];
    bool found = false;
    
    while (fgets(line, sizeof(line), f)) {
        PlayerStats temp;
        int scanned = sscanf(line, "%31[^,],%u,%u,%lu,%u,%u,%lu,%lu",
            temp.username,
            &temp.games_played,
            &temp.games_won,
            &temp.total_score,
            &temp.total_correct_guesses,
            &temp.total_rounds_drawn,
            &temp.fastest_guess_ms,
            &temp.last_played);
        
        if (scanned >= 7 && strcmp(temp.username, username) == 0) {
            memcpy(stats, &temp, sizeof(PlayerStats));
            found = true;
            break;
        }
    }
    
    fclose(f);
    pthread_mutex_unlock(&stats_mutex);
    
    if (!found) {
        // Initialize new player stats
        memset(stats, 0, sizeof(PlayerStats));
        strncpy(stats->username, username, MAX_USERNAME - 1);
        stats->fastest_guess_ms = UINT64_MAX;
        return -1;
    }
    
    printf("[STATS] Loaded stats for %s: %u games, %lu total score\n",
           username, stats->games_played, stats->total_score);
    return 0;
}

void save_player_stats(const PlayerStats* stats) {
    pthread_mutex_lock(&stats_mutex);
    
    FILE* f_read = fopen(STATS_FILE, "r");
    FILE* f_write = fopen(TEMP_STATS_FILE, "w");
    
    if (!f_write) {
        fprintf(stderr, "[STATS] Error: Cannot write stats file\n");
        if (f_read) fclose(f_read);
        pthread_mutex_unlock(&stats_mutex);
        return;
    }
    
    bool updated = false;
    char line[512];
    
    // Copy all lines, updating the matching username
    if (f_read) {
        while (fgets(line, sizeof(line), f_read)) {
            char existing_username[MAX_USERNAME];
            sscanf(line, "%31[^,]", existing_username);
            
            if (strcmp(existing_username, stats->username) == 0) {
                // Write updated stats
                fprintf(f_write, "%s,%u,%u,%lu,%u,%u,%lu,%lu\n",
                    stats->username,
                    stats->games_played,
                    stats->games_won,
                    stats->total_score,
                    stats->total_correct_guesses,
                    stats->total_rounds_drawn,
                    stats->fastest_guess_ms,
                    stats->last_played);
                updated = true;
            } else {
                // Copy existing line
                fputs(line, f_write);
            }
        }
        fclose(f_read);
    }
    
    // If username not found, append new entry
    if (!updated) {
        fprintf(f_write, "%s,%u,%u,%lu,%u,%u,%lu,%lu\n",
            stats->username,
            stats->games_played,
            stats->games_won,
            stats->total_score,
            stats->total_correct_guesses,
            stats->total_rounds_drawn,
            stats->fastest_guess_ms,
            stats->last_played);
    }
    
    fclose(f_write);
    
    // Replace old file with new file
    remove(STATS_FILE);
    rename(TEMP_STATS_FILE, STATS_FILE);
    
    pthread_mutex_unlock(&stats_mutex);
    
    printf("[STATS] Saved stats for %s: %u games, %lu total score\n",
           stats->username, stats->games_played, stats->total_score);
}

void update_game_stats(Player* player, bool won, int correct_guesses, int rounds_drawn) {
    PlayerStats stats;
    
    // Load existing stats
    load_player_stats(player->username, &stats);
    
    // Update stats
    stats.games_played++;
    if (won) {
        stats.games_won++;
    }
    stats.total_score += player->score;
    stats.total_correct_guesses += correct_guesses;
    stats.total_rounds_drawn += rounds_drawn;
    stats.last_played = get_current_time_ms();
    
    // Save updated stats
    save_player_stats(&stats);
    
    printf("[STATS] Updated stats for %s: Games: %u, Wins: %u, Score: %lu\n",
           player->username, stats.games_played, stats.games_won, stats.total_score);
}

void update_fastest_guess(Player* player, uint64_t guess_time_ms) {
    PlayerStats stats;
    
    // Load existing stats
    load_player_stats(player->username, &stats);
    
    // Update if this is a new record
    if (guess_time_ms < stats.fastest_guess_ms) {
        stats.fastest_guess_ms = guess_time_ms;
        save_player_stats(&stats);
        
        printf("[STATS] New fastest guess record for %s: %lu ms\n",
               player->username, guess_time_ms);
    }
}

int get_leaderboard(PlayerStats* leaderboard, int max_count) {
    pthread_mutex_lock(&stats_mutex);
    
    FILE* f = fopen(STATS_FILE, "r");
    if (!f) {
        pthread_mutex_unlock(&stats_mutex);
        return 0;
    }
    
    // Read all stats into temporary array
    PlayerStats all_stats[1000];  // Max 1000 players
    int count = 0;
    char line[512];
    
    while (fgets(line, sizeof(line), f) && count < 1000) {
        PlayerStats temp;
        int scanned = sscanf(line, "%31[^,],%u,%u,%lu,%u,%u,%lu,%lu",
            temp.username,
            &temp.games_played,
            &temp.games_won,
            &temp.total_score,
            &temp.total_correct_guesses,
            &temp.total_rounds_drawn,
            &temp.fastest_guess_ms,
            &temp.last_played);
        
        if (scanned >= 7) {
            all_stats[count++] = temp;
        }
    }
    
    fclose(f);
    
    // Sort by total_score (bubble sort - simple for small datasets)
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (all_stats[j].total_score < all_stats[j + 1].total_score) {
                PlayerStats temp = all_stats[j];
                all_stats[j] = all_stats[j + 1];
                all_stats[j + 1] = temp;
            }
        }
    }
    
    // Copy top N to leaderboard
    int result_count = (count < max_count) ? count : max_count;
    for (int i = 0; i < result_count; i++) {
        leaderboard[i] = all_stats[i];
    }
    
    pthread_mutex_unlock(&stats_mutex);
    return result_count;
}
