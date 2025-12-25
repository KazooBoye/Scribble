#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "protocol.h"
#include "tcp/tcp_server.h"
#include "tcp/tcp_handler.h"
#include "udp/udp_server.h"
#include "game/game_logic.h"
#include "game/matchmaking.h"
#include "game/reconnection.h"
#include "utils/logger.h"
#include "utils/timer.h"

static volatile bool server_running = true;

void signal_handler(int signum) {
    printf("\n[SERVER] Received signal %d, shutting down...\n", signum);
    server_running = false;
}

void timer_update_callback(Room* room) {
    check_game_start_countdown(room);
    update_timer(room);
    
    // Broadcast timer update to all players
    char timer_msg[128];
    snprintf(timer_msg, sizeof(timer_msg), 
             "{\"time_remaining\":%d}", room->time_remaining);
    broadcast_to_room(room, MSG_TIMER_UPDATE, timer_msg, NULL);
}

void* timer_thread(void* arg) {
    (void)arg;
    
    while (server_running) {
        sleep_ms(1000);  // Update every second
        
        // Update timers for all active rooms
        iterate_active_rooms(timer_update_callback);
        
        // Cleanup expired states
        cleanup_expired_states();
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    (void)argc;  // Unused parameter
    (void)argv;  // Unused parameter
    printf("╔══════════════════════════════════════════╗\n");
    printf("║   Scribble Game Server - Starting...    ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize logger
    if (logger_init("server/server.log") < 0) {
        fprintf(stderr, "[ERROR] Failed to initialize logger\n");
        return 1;
    }
    printf("[SERVER] Logger initialized\n");
    
    // Load word list
    if (load_word_list("server/game/wordlist.txt") < 0) {
        fprintf(stderr, "[ERROR] Failed to load word list\n");
        fprintf(stderr, "[HINT] Make sure to run 'make install' or 'make all'\n");
        logger_close();
        return 1;
    }
    printf("[SERVER] Word list loaded\n");
    
    // Initialize game systems
    init_matchmaking();
    printf("[SERVER] Matchmaking system initialized\n");
    
    init_reconnection();
    printf("[SERVER] Reconnection system initialized\n");
    
    // Start TCP server
    if (tcp_server_start(TCP_PORT) < 0) {
        fprintf(stderr, "[ERROR] Failed to start TCP server\n");
        logger_close();
        return 1;
    }
    
    // Start UDP server
    if (udp_server_start(UDP_PORT) < 0) {
        fprintf(stderr, "[ERROR] Failed to start UDP server\n");
        tcp_server_stop();
        logger_close();
        return 1;
    }
    
    // Start timer thread
    pthread_t timer_tid;
    pthread_create(&timer_tid, NULL, timer_thread, NULL);
    
    printf("\n╔══════════════════════════════════════════╗\n");
    printf("║      Server is running successfully!     ║\n");
    printf("╠══════════════════════════════════════════╣\n");
    printf("║  TCP (Game):    port %d                 ║\n", TCP_PORT);
    printf("║  UDP (Drawing): port %d                 ║\n", UDP_PORT);
    printf("╚══════════════════════════════════════════╝\n\n");
    printf("[SERVER] Press Ctrl+C to stop\n\n");
    
    // Main loop
    while (server_running) {
        sleep(1);
    }
    
    // Cleanup
    printf("\n[SERVER] Shutting down...\n");
    
    udp_server_stop();
    tcp_server_stop();
    
    pthread_join(timer_tid, NULL);
    
    cleanup_word_list();
    logger_close();
    
    printf("[SERVER] Shutdown complete\n");
    return 0;
}
