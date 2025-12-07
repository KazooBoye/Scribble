#include "ws_thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

// Cross-platform SHA1 support
#ifdef __APPLE__
    #include <CommonCrypto/CommonDigest.h>
    #define SHA1_DIGEST_LENGTH CC_SHA1_DIGEST_LENGTH
    #define sha1_compute(data, len, result) CC_SHA1((unsigned char*)(data), (CC_LONG)(len), (result))
#else
    #include <openssl/sha.h>
    #define SHA1_DIGEST_LENGTH SHA_DIGEST_LENGTH
    #define sha1_compute(data, len, result) SHA1((unsigned char*)(data), (len), (result))
#endif

#define MAX_WS_CLIENTS 50

typedef struct {
    int ws_fd;           // WebSocket file descriptor
    int tcp_fd;          // TCP connection to game server
    bool active;
    int client_id;
} WSClient;

static WSClient ws_clients[MAX_WS_CLIENTS];
static int ws_client_count = 0;
static int next_client_id = 1;

// TCP server address (shared config)
static char tcp_server_host[256] = "127.0.0.1";
static int tcp_server_port = 9090;

// Create TCP connection to game server for a client
int create_tcp_connection() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Failed to create TCP socket");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(tcp_server_port);
    inet_pton(AF_INET, tcp_server_host, &server_addr.sin_addr);
    
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to game server");
        close(sock_fd);
        return -1;
    }
    
    return sock_fd;
}

// Serialize TCP message with length prefix (local helper)
int ws_serialize_tcp_message(const char* json, char* buffer, int buffer_size) {
    int json_len = strlen(json);
    if (json_len + 4 > buffer_size) return -1;
    
    uint32_t len_network = htonl(json_len);
    memcpy(buffer, &len_network, 4);
    memcpy(buffer + 4, json, json_len);
    
    return 4 + json_len;
}

// Base64 encoding table
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64 encode
void base64_encode(const unsigned char* input, int length, char* output) {
    int i = 0, j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    while (length--) {
        char_array_3[i++] = *(input++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (i = 0; i < 4; i++)
                output[j++] = base64_table[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        int k;
        for (k = i; k < 3; k++)
            char_array_3[k] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        
        for (k = 0; k < i + 1; k++)
            output[j++] = base64_table[char_array_4[k]];
        
        while (i++ < 3)
            output[j++] = '=';
    }
    output[j] = '\0';
}

// WebSocket handshake with proper Sec-WebSocket-Accept calculation
int ws_handshake(int client_fd) {
    char buffer[2048];
    int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) return -1;
    
    buffer[bytes] = '\0';
    
    // Find Sec-WebSocket-Key
    const char* key_header = "Sec-WebSocket-Key: ";
    char* key_start = strstr(buffer, key_header);
    if (!key_start) return -1;
    
    key_start += strlen(key_header);
    char* key_end = strstr(key_start, "\r\n");
    if (!key_end) return -1;
    
    char key[128];
    int key_len = key_end - key_start;
    strncpy(key, key_start, key_len);
    key[key_len] = '\0';
    
    // Compute Sec-WebSocket-Accept
    // Concatenate key + magic GUID
    const char* magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char concat[256];
    snprintf(concat, sizeof(concat), "%s%s", key, magic);
    
    // SHA-1 hash
    unsigned char sha1_result[SHA1_DIGEST_LENGTH];
    sha1_compute(concat, strlen(concat), sha1_result);
    
    // Base64 encode
    char accept_key[64];
    base64_encode(sha1_result, SHA1_DIGEST_LENGTH, accept_key);
    
    // Build response
    char response[512];
    snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        accept_key);
    
    send(client_fd, response, strlen(response), 0);
    return 0;
}

// Simple WebSocket frame decoder (text frames only, simplified)
int ws_decode_frame(const char* buffer, int len, char* payload, int* payload_len) {
    if (len < 2) return -1;
    
    unsigned char opcode = buffer[0] & 0x0F;
    bool masked = (buffer[1] & 0x80) != 0;
    int payload_length = buffer[1] & 0x7F;
    
    // Handle control frames (ping=0x9, pong=0xA, close=0x8)
    if (opcode == 0x8 || opcode == 0x9 || opcode == 0xA) {
        // Skip control frames for now
        *payload_len = 0;
        return -1;  // Signal to ignore this frame
    }
    
    // Only process text frames (0x1) and continuation frames (0x0)
    if (opcode != 0x1 && opcode != 0x0) {
        *payload_len = 0;
        return -1;
    }
    
    int offset = 2;
    
    if (payload_length == 126) {
        if (len < 4) return -1;
        payload_length = (buffer[2] << 8) | buffer[3];
        offset = 4;
    } else if (payload_length == 127) {
        // Extended payload length not supported in this simple version
        return -1;
    }
    
    if (masked) {
        if (len < offset + 4) return -1;
        unsigned char mask[4];
        memcpy(mask, buffer + offset, 4);
        offset += 4;
        
        if (len < offset + payload_length) return -1;
        
        for (int i = 0; i < payload_length; i++) {
            payload[i] = buffer[offset + i] ^ mask[i % 4];
        }
    } else {
        if (len < offset + payload_length) return -1;
        memcpy(payload, buffer + offset, payload_length);
    }
    
    *payload_len = payload_length;
    return offset + payload_length;
}

// Simple WebSocket frame encoder (text frames, unmasked)
int ws_encode_frame(const char* payload, int payload_len, char* frame, int frame_size) {
    if (frame_size < payload_len + 10) return -1;
    
    int offset = 0;
    frame[offset++] = 0x81;  // FIN + text frame
    
    if (payload_len < 126) {
        frame[offset++] = payload_len;
    } else if (payload_len < 65536) {
        frame[offset++] = 126;
        frame[offset++] = (payload_len >> 8) & 0xFF;
        frame[offset++] = payload_len & 0xFF;
    } else {
        return -1;  // Not supported
    }
    
    memcpy(frame + offset, payload, payload_len);
    return offset + payload_len;
}

int ws_thread_init(WSThread* ws, Dispatcher* dispatcher, int port) {
    ws->dispatcher = dispatcher;
    ws->port = port;
    ws->running = true;
    
    memset(ws_clients, 0, sizeof(ws_clients));
    
    if (pthread_create(&ws->thread, NULL, ws_thread_func, ws) != 0) {
        perror("Failed to create WebSocket thread");
        return -1;
    }
    
    printf("[WS] Thread started on port %d\n", port);
    return 0;
}

void ws_thread_destroy(WSThread* ws) {
    ws->running = false;
    pthread_join(ws->thread, NULL);
    
    // Close all client connections
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        if (ws_clients[i].active) {
            if (ws_clients[i].ws_fd > 0) close(ws_clients[i].ws_fd);
            if (ws_clients[i].tcp_fd > 0) close(ws_clients[i].tcp_fd);
            ws_clients[i].active = false;
        }
    }
    
    printf("[WS] Thread stopped\n");
}

void* ws_thread_func(void* arg) {
    WSThread* ws = (WSThread*)arg;
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Failed to create WebSocket server socket");
        return NULL;
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(ws->port);
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Failed to bind WebSocket server");
        close(server_fd);
        return NULL;
    }
    
    if (listen(server_fd, 10) < 0) {
        perror("Failed to listen on WebSocket server");
        close(server_fd);
        return NULL;
    }
    
    printf("[WS] Listening on port %d\n", ws->port);
    
    while (ws->running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        
        int max_fd = server_fd;
        for (int i = 0; i < MAX_WS_CLIENTS; i++) {
            if (ws_clients[i].active) {
                if (ws_clients[i].ws_fd > 0) {
                    FD_SET(ws_clients[i].ws_fd, &read_fds);
                    if (ws_clients[i].ws_fd > max_fd) {
                        max_fd = ws_clients[i].ws_fd;
                    }
                }
                if (ws_clients[i].tcp_fd > 0) {
                    FD_SET(ws_clients[i].tcp_fd, &read_fds);
                    if (ws_clients[i].tcp_fd > max_fd) {
                        max_fd = ws_clients[i].tcp_fd;
                    }
                }
            }
        }
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0 && errno != EINTR) continue;
        
        // New connection
        if (FD_ISSET(server_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_fd >= 0) {
                if (ws_handshake(client_fd) == 0) {
                    // Create TCP connection to game server for this client
                    int tcp_fd = create_tcp_connection();
                    if (tcp_fd < 0) {
                        printf("[WS] Failed to create TCP connection for client\n");
                        close(client_fd);
                    } else {
                        // Add to client list
                        for (int i = 0; i < MAX_WS_CLIENTS; i++) {
                            if (!ws_clients[i].active) {
                                ws_clients[i].ws_fd = client_fd;
                                ws_clients[i].tcp_fd = tcp_fd;
                                ws_clients[i].active = true;
                                ws_clients[i].client_id = next_client_id++;
                                ws_client_count++;
                                printf("[WS] Client %d connected (WS:%d, TCP:%d)\n", 
                                       ws_clients[i].client_id, client_fd, tcp_fd);
                                break;
                            }
                        }
                    }
                } else {
                    close(client_fd);
                }
            }
        }
        
        // Handle client messages
        for (int i = 0; i < MAX_WS_CLIENTS; i++) {
            if (!ws_clients[i].active) continue;
            
            // Handle WebSocket messages (browser -> server)
            if (ws_clients[i].ws_fd > 0 && FD_ISSET(ws_clients[i].ws_fd, &read_fds)) {
                char buffer[BUFFER_SIZE];
                int bytes = recv(ws_clients[i].ws_fd, buffer, sizeof(buffer), 0);
                
                if (bytes <= 0) {
                    // Disconnected
                    printf("[WS] Client %d disconnected\n", ws_clients[i].client_id);
                    close(ws_clients[i].ws_fd);
                    if (ws_clients[i].tcp_fd > 0) close(ws_clients[i].tcp_fd);
                    ws_clients[i].active = false;
                    ws_client_count--;
                } else {
                    // Decode WebSocket frame
                    char payload[BUFFER_SIZE];
                    int payload_len;
                    if (ws_decode_frame(buffer, bytes, payload, &payload_len) > 0) {
                        payload[payload_len] = '\0';
                        printf("[WS] Client %d sent: %s\n", ws_clients[i].client_id, payload);
                        
                        // Forward directly to TCP (serialize with length prefix)
                        char tcp_buffer[BUFFER_SIZE];
                        int tcp_len = ws_serialize_tcp_message(payload, tcp_buffer, sizeof(tcp_buffer));
                        if (tcp_len > 0 && ws_clients[i].tcp_fd > 0) {
                            send(ws_clients[i].tcp_fd, tcp_buffer, tcp_len, 0);
                        }
                    }
                }
            }
            
            // Handle TCP responses (server -> browser)
            if (ws_clients[i].tcp_fd > 0 && FD_ISSET(ws_clients[i].tcp_fd, &read_fds)) {
                char buffer[BUFFER_SIZE];
                int bytes = recv(ws_clients[i].tcp_fd, buffer, sizeof(buffer), 0);
                
                if (bytes <= 0) {
                    printf("[TCP] Connection closed for client %d\n", ws_clients[i].client_id);
                    close(ws_clients[i].tcp_fd);
                    ws_clients[i].tcp_fd = -1;
                } else {
                    // Deserialize ALL length-prefixed messages in the buffer
                    int offset = 0;
                    while (offset + 4 <= bytes) {
                        uint32_t json_len;
                        memcpy(&json_len, buffer + offset, 4);
                        json_len = ntohl(json_len);
                        
                        // Check if we have the complete message
                        if (offset + 4 + (int)json_len <= bytes) {
                            char* json_payload = buffer + offset + 4;
                            printf("[TCP] Client %d received: %.*s\n", ws_clients[i].client_id, (int)json_len, json_payload);
                            
                            // Encode as WebSocket frame and send to browser
                            char ws_frame[BUFFER_SIZE];
                            int frame_len = ws_encode_frame(json_payload, json_len, ws_frame, sizeof(ws_frame));
                            if (frame_len > 0 && ws_clients[i].ws_fd > 0) {
                                send(ws_clients[i].ws_fd, ws_frame, frame_len, 0);
                            }
                            
                            // Move to next message
                            offset += 4 + json_len;
                        } else {
                            // Incomplete message, break and wait for more data
                            printf("[TCP] Client %d: Incomplete message in buffer (need %u bytes, have %d)\n", 
                                   ws_clients[i].client_id, json_len, bytes - offset - 4);
                            break;
                        }
                    }
                }
            }
        }
        
        // Note: Direct TCP<->WebSocket forwarding now, no dispatcher queues needed
    }
    
    close(server_fd);
    return NULL;
}
