/*
 * Scribble Game - Client Networking Library (C)
 * Pure C implementation of TCP/UDP sockets for client-server communication
 * Exposes simple C API that can be called from Python via ctypes
 */

#ifndef CLIENT_NETWORK_H
#define CLIENT_NETWORK_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Callback function type for received messages
// Parameters: message_json (null-terminated string)
typedef void (*message_callback_t)(const char* message_json);

/**
 * Initialize and connect to server
 * Returns: 0 on success, -1 on failure
 */
int network_connect(const char* host, int tcp_port, int udp_port);

/**
 * Send message via TCP (reliable, for game logic)
 * message_json: null-terminated JSON string
 * Returns: 0 on success, -1 on failure
 */
int network_send_tcp(const char* message_json);

/**
 * Send message via UDP (fast, for drawing strokes)
 * message_json: null-terminated JSON string
 * Returns: 0 on success, -1 on failure
 */
int network_send_udp(const char* message_json);

/**
 * Register callback for received messages
 * This callback will be invoked from the receive thread
 * The callback should be thread-safe and process messages quickly
 */
void network_set_callback(message_callback_t callback);

/**
 * Check if connected to server
 * Returns: 1 if connected, 0 if not
 */
int network_is_connected(void);

/**
 * Disconnect and cleanup
 */
void network_disconnect(void);

/**
 * Get last error message
 * Returns: pointer to static error string
 */
const char* network_get_error(void);

#ifdef __cplusplus
}
#endif

#endif // CLIENT_NETWORK_H
