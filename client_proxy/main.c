#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "protocol.h"
#include "threads/dispatcher.h"
#include "threads/ws_thread.h"
#include "threads/tcp_thread.h"
#include "threads/udp_thread.h"

static volatile bool proxy_running = true;

void signal_handler(int signum) {
    printf("\n[PROXY] Received signal %d, shutting down...\n", signum);
    proxy_running = false;
}

int main(int argc, char* argv[]) {
    char* server_host = "127.0.0.1";
    
    if (argc > 1) {
        server_host = argv[1];
    }
    
    printf("╔══════════════════════════════════════════╗\n");
    printf("║   Scribble Client Proxy - Starting...   ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize dispatcher (central routing)
    Dispatcher dispatcher;
    if (dispatcher_init(&dispatcher) < 0) {
        fprintf(stderr, "[ERROR] Failed to initialize dispatcher\n");
        return 1;
    }
    
    // Start Thread 1: WebSocket listener
    WSThread ws_thread;
    if (ws_thread_init(&ws_thread, &dispatcher, WS_PORT) < 0) {
        fprintf(stderr, "[ERROR] Failed to start WebSocket thread\n");
        dispatcher_destroy(&dispatcher);
        return 1;
    }
    
    // Start Thread 2: TCP handler (connection to game server)
    TCPThread tcp_thread;
    if (tcp_thread_init(&tcp_thread, &dispatcher, server_host, TCP_PORT) < 0) {
        fprintf(stderr, "[ERROR] Failed to start TCP thread\n");
        ws_thread_destroy(&ws_thread);
        dispatcher_destroy(&dispatcher);
        return 1;
    }
    
    // Start Thread 3: UDP handler (drawing strokes)
    UDPThread udp_thread;
    if (udp_thread_init(&udp_thread, &dispatcher, server_host, UDP_PORT) < 0) {
        fprintf(stderr, "[ERROR] Failed to start UDP thread\n");
        tcp_thread_destroy(&tcp_thread);
        ws_thread_destroy(&ws_thread);
        dispatcher_destroy(&dispatcher);
        return 1;
    }
    
    // Thread 4 is the dispatcher itself, already running
    
    printf("\n╔══════════════════════════════════════════╗\n");
    printf("║    Proxy is running successfully!        ║\n");
    printf("╠══════════════════════════════════════════╣\n");
    printf("║  WebSocket:     port %d                 ║\n", WS_PORT);
    printf("║  Game Server:   %s:%d            ║\n", server_host, TCP_PORT);
    printf("║  Drawing (UDP): %s:%d            ║\n", server_host, UDP_PORT);
    printf("╠══════════════════════════════════════════╣\n");
    printf("║  Multi-threaded Architecture:            ║\n");
    printf("║    Thread 1: WebSocket Listener          ║\n");
    printf("║    Thread 2: TCP Handler (Game)          ║\n");
    printf("║    Thread 3: UDP Handler (Drawing)       ║\n");
    printf("║    Thread 4: Dispatcher (Routing)        ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");
    printf("[PROXY] Press Ctrl+C to stop\n\n");
    
    // Main loop
    while (proxy_running) {
        sleep(1);
    }
    
    // Cleanup
    printf("\n[PROXY] Shutting down...\n");
    
    udp_thread_destroy(&udp_thread);
    tcp_thread_destroy(&tcp_thread);
    ws_thread_destroy(&ws_thread);
    dispatcher_destroy(&dispatcher);
    
    printf("[PROXY] Shutdown complete\n");
    return 0;
}
