#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Constants
#define MAX_PLAYERS 5
#define MAX_USERNAME 32
#define MAX_WORD_LEN 32
#define MAX_CHAT_LEN 256
#define MAX_ROOMS 100
#define ROUND_TIME 90
#define RECONNECT_TIMEOUT 300  // 5 minutes
#define MAX_CHAT_HISTORY 10
#define MAX_STROKES 10000
#define BUFFER_SIZE 4096

// Ports
#define TCP_PORT 9090
#define UDP_PORT 9091

// Message Types (TCP)
typedef enum {
    MSG_PING = 0,
    MSG_PONG,
    MSG_REGISTER,
    MSG_REGISTER_ACK,
    MSG_JOIN_ROOM,
    MSG_CREATE_ROOM,
    MSG_ROOM_CREATED,
    MSG_ROOM_JOINED,
    MSG_ROOM_FULL,
    MSG_ROOM_NOT_FOUND,
    MSG_GAME_START,
    MSG_YOUR_TURN,
    MSG_WORD_TO_DRAW,
    MSG_ROUND_START,
    MSG_CHAT,
    MSG_CHAT_BROADCAST,
    MSG_GUESS_CORRECT,
    MSG_GUESS_WRONG,
    MSG_TIMER_UPDATE,
    MSG_COUNTDOWN_UPDATE,
    MSG_ROUND_END,
    MSG_GAME_END,
    MSG_PLAYER_JOIN,
    MSG_PLAYER_LEAVE,
    MSG_SCORE_UPDATE,
    MSG_RECONNECT_REQUEST,
    MSG_RECONNECT_SUCCESS,
    MSG_RECONNECT_FAIL,
    MSG_ERROR,
    MSG_DISCONNECT
} MessageType;

// UDP Message Types
typedef enum {
    UDP_STROKE = 100,
    UDP_CLEAR_CANVAS,
    UDP_UNDO
} UDPMessageType;

// Player State
typedef enum {
    PLAYER_LOBBY = 0,
    PLAYER_IN_ROOM,
    PLAYER_PLAYING,
    PLAYER_DRAWING,
    PLAYER_GUESSING,
    PLAYER_DISCONNECTED
} PlayerState;

// Room State
typedef enum {
    ROOM_WAITING = 0,
    ROOM_PLAYING,
    ROOM_ENDED
} RoomState;

// Stroke structure for UDP
typedef struct {
    uint32_t stroke_id;
    float x1, y1, x2, y2;
    uint32_t color;
    uint8_t thickness;
    uint64_t timestamp;
} Stroke;

// Player structure
typedef struct {
    int fd;  // TCP socket
    char username[MAX_USERNAME];
    char ip[INET_ADDRSTRLEN];
    uint32_t player_id;
    int score;
    PlayerState state;
    uint64_t rtt;  // Round trip time in milliseconds
    char session_token[64];
    uint64_t last_seen;
    bool is_drawing;
    bool has_guessed;
    bool has_drawn;  // Track if player has had their turn to draw
    // Game session tracking (for stats)
    int correct_guesses_this_game;
    int rounds_drawn_this_game;
    uint64_t round_start_time;  // For tracking guess speed
    // TCP receive buffer for handling partial messages
    char recv_buffer[BUFFER_SIZE];
    int recv_buffer_len;
} Player;

// Room structure
typedef struct {
    uint32_t room_id;
    char room_code[16];
    Player* players[MAX_PLAYERS];
    int player_count;
    RoomState state;
    int current_drawer_idx;
    char current_word[MAX_WORD_LEN];
    int round_number;
    int total_rounds;  // Total rounds for this game (equals player count at start)
    uint64_t round_start_time;
    int time_remaining;
    Stroke strokes[MAX_STROKES];
    int stroke_count;
    bool is_private;
    uint64_t created_at;
    uint64_t game_start_countdown;  // Timestamp when countdown started (0 = not started)
    bool countdown_active;           // Whether countdown is active
} Room;

// TCP Message Header (4 bytes length + JSON payload)
typedef struct {
    uint32_t length;
    char* json_data;
} TCPMessage;

// UDP Message Header
typedef struct {
    UDPMessageType type;
    uint32_t room_id;
    union {
        Stroke stroke;
    } data;
} UDPMessage;

// Function prototypes for message serialization
int serialize_tcp_message(MessageType type, const char* json, char* buffer, int buffer_size);
int deserialize_tcp_message(const char* buffer, int len, MessageType* type, char** json);
int serialize_udp_stroke(const Stroke* stroke, uint32_t room_id, char* buffer, int buffer_size);
int deserialize_udp_stroke(const char* buffer, int len, Stroke* stroke, uint32_t* room_id);

// JSON parsing helpers
int json_get_data_string(const char* json, const char* key, char* out, int out_size);
int json_get_data_int(const char* json, const char* key, int* out);

#endif // PROTOCOL_H
