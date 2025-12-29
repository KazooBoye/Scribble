# BÁO CÁO KỸ THUẬT DỰ ÁN SCRIBBLE
## Multiplayer Drawing & Guessing Game

**Ngày báo cáo:** 26/12/2025  
**Môn học:** Lập trình mạng (Network Programming)  
**Người thực hiện:** Cao Duc Anh

**Cập nhật gần đây:**
- ✅ Persistent Player Statistics (CSV storage)
- ✅ Host Controls (early game start, kick players for private rooms)
- ✅ Resource Manager (programmatic fallback icon generation)
- ✅ Simplified Connection Monitoring (PING/PONG with TCP resilience)

---

## 📋 MỤC LỤC

1. [Tổng quan dự án](#1-tổng-quan-dự-án)
2. [Kiến trúc hệ thống](#2-kiến-trúc-hệ-thống)
3. [Use Cases](#3-use-cases)
4. [Cấu trúc mã nguồn](#4-cấu-trúc-mã-nguồn)
5. [Các thành phần chính](#5-các-thành-phần-chính)
6. [Protocol và Message Flow](#6-protocol-và-message-flow)
7. [Tính năng đã hoàn thành](#7-tính-năng-đã-hoàn-thành)
8. [Kết luận](#8-kết-luận)
9. [Phụ lục](#phụ-lục)

---

## 1. TỔNG QUAN DỰ ÁN

### 1.1. Mô tả dự án
Scribble là một trò chơi multiplayer real-time tương tự skribbl.io, được xây dựng với C backend server kết hợp với Pygame client. Server sử dụng C thuần với POSIX threads và Berkeley sockets, trong khi client sử dụng Python/Pygame với C networking library thông qua ctypes.

### 1.2. Mục tiêu
- Xây dựng game server xử lý logic game và quản lý phòng chơi
- Phát triển Pygame client với C networking library
- Triển khai hệ thống matchmaking thông minh
- Đảm bảo đồng bộ real-time cho drawing và chat qua TCP
- Connection monitoring với PING/PONG
- Tạo UI trực quan với Pygame rendering

---

## 2. KIẾN TRÚC HỆ THỐNG

### 2.1. Sơ đồ kiến trúc tổng quan

```
┌─────────────────────────────────────────────────────────────┐
│              PYGAME CLIENTS (Python + Pygame)               │
│         Pygame UI + Canvas + ctypes → C Library             │
└──────────────────┬──────────────────────────────────────────┘
                   │ ctypes FFI
                   ▼
┌─────────────────────────────────────────────────────────────┐
│           C NETWORKING LIBRARY (libscribble_client)         │
│              network.c - Berkeley Sockets                   │
│              - TCP socket management                        │
│              - Message framing (4-byte length prefix)       │
│              - Compiled to .dylib (macOS) / .so (Linux)     │
│              - Called via Python ctypes                     │
└──────────────────┬──────────────────────────────────────────┘
                   │ TCP (Port 9090)
                   │ JSON over TCP
                   ▼
┌─────────────────────────────────────────────────────────────┐
│                      GAME SERVER (C Program)                │
├─────────────────┬──────────────┬──────────────┬─────────────┤
│  TCP Server     │  Game Engine │  Matchmaking │  Timer      │
│  (Port 9090)    │              │              │  Thread     │
│  - Per-client   │  - Rooms     │  - Auto      │  - 1Hz      │
│    sockets      │  - Rounds    │    match     │    tick     │
│  - select()     │  - Scoring   │  - Private   │  - Round    │
│  - Broadcast    │  - Drawing   │    rooms     │    timer    │
│  - JSON parse   │    state     │  - Join code │  - Updates  │
├─────────────────┴──────────────┴──────────────┴─────────────┤
│  Stats System   │  Connection   │  Host Controls             │
│  - CSV storage  │  Monitoring   │  - Early start             │
│  - Thread-safe  │  - PING/PONG  │  - Kick players            │
│  - Leaderboard  │  - 15s timeout│  - Permission check        │
└─────────────────┴───────────────┴────────────────────────────┘
```

### 2.2. Luồng dữ liệu

**Game State Flow:**
```
Pygame → ctypes → C Library → TCP → Server
                                       ↓
                                 Game Logic
                                       ↓
Server → TCP → C Library → ctypes → Pygame
```

**Drawing Flow (TCP with JSON):**
```
Drawer Pygame → C Library → TCP → Server
                                      ↓
                              Broadcast to room
                                      ↓
Server → TCP → C Library → Other Pygame Clients
```

**Message Format:**
```
[4-byte length (big-endian)][JSON payload]

Example:
[0x00, 0x00, 0x00, 0x3A] {"type":2,"data":{"username":"Alice"}}
```

---

## 3. USE CASES

### 3.1. Use Case: Join Game via Auto Matchmaking

**Actor:** Người chơi  
**Precondition:** Server đang chạy, Pygame client đã build  

**Main Flow:**
1. Người chơi chạy `python3 client_pygame/main.py --host localhost --port 9090`
2. Pygame window hiển thị landing screen
3. Nhập username và click "Play Now"
4. Client gửi `MSG_REGISTER` → Server qua TCP
5. Server trả về `MSG_REGISTER_ACK` với `player_id` và `session_token`
6. Client gửi `MSG_JOIN_ROOM` với room_id = 0 (auto matchmaking)
7. Server tìm room phù hợp hoặc tạo room mới
8. Server trả về `MSG_ROOM_JOINED` với thông tin room
9. Khi đủ 2+ người chơi, countdown 15 giây bắt đầu
10. Server broadcast `MSG_COUNTDOWN_UPDATE` mỗi giây
11. Sau 15 giây, game bắt đầu với `MSG_GAME_START`

**Postcondition:** Người chơi vào room và chờ game bắt đầu

### 3.2. Use Case: Drawing Phase

**Actor:** Người chơi đang vẽ (Drawer)  
**Precondition:** Game đang chạy, đến lượt người chơi vẽ  

**Main Flow:**
1. Server gửi `MSG_YOUR_TURN` và `MSG_WORD_TO_DRAW` cho drawer
2. Drawer nhận được từ cần vẽ, hiển thị ở top of canvas
3. UI hiển thị drawing tools (color palette, brush size slider)
4. Drawer vẽ trên canvas bằng Pygame:
   - Mouse drag → tạo stroke {x1, y1, x2, y2, color, thickness}
   - Client gửi `MSG_STROKE` (type 100) qua TCP với JSON data
5. Server nhận stroke và broadcast cho tất cả players khác trong room
6. Other players nhận stroke và vẽ trên Pygame canvas
7. Drawing đồng bộ real-time cho tất cả players

**Alternative Flow:**
- Drawer click "Clear Canvas" → Server broadcast `MSG_CLEAR_CANVAS` (type 101)

### 3.3. Use Case: Guessing Phase

**Actor:** Người chơi đang đoán (Guesser)  
**Precondition:** Game đang chạy, người khác đang vẽ  

**Main Flow:**
1. Guesser nhìn canvas và nhập guess vào chat
2. Client gửi `MSG_CHAT` với message
3. Server so sánh guess với current_word (case-insensitive)
4. **Nếu đúng:**
   - Đánh dấu `player.has_guessed = true`
   - Tính điểm dựa trên thời gian còn lại: `points = 10 + (time_remaining * 90 / ROUND_TIME)`
   - Server broadcast `MSG_GUESS_CORRECT` với username và score
   - Client hiển thị notification màu xanh: "Player X guessed correctly! 🎉"
5. **Nếu sai:**
   - Server broadcast `MSG_CHAT_BROADCAST` để hiển thị chat bình thường

**Special Case:**
- Tất cả players đã đoán đúng → End round sớm

### 3.4. Use Case: Connection Monitoring & Player Disconnect

**Actor:** Người chơi trong game  
**Precondition:** Game đang chơi, player đã kết nối  

**Main Flow - Connection Monitoring:**
1. Client gửi `MSG_PING` mỗi 5 giây
2. Server nhận PING và trả về `MSG_PONG`
3. Client track thời gian chờ PONG response
4. Nếu không nhận PONG trong 15 giây → connection timeout
5. Client hiển thị red banner "Connection Lost"
6. TCP layer cố gắng duy trì connection (OS-level buffering)
7. Nếu kết nối phục hồi → Client nhận PONG → ẩn banner
8. Nếu kết nối không phục hồi → TCP disconnect

**Main Flow - Permanent Disconnect:**
1. TCP connection bị đóng (timeout, network failure, hoặc user close)
2. Server phát hiện socket disconnect (select() hoặc send error)
3. Server xóa player khỏi room
4. Server broadcast `MSG_PLAYER_LEAVE` cho players còn lại
5. Nếu player là drawer → End round và chuyển lượt
6. Game tiếp tục với players còn lại

**Reconnection Behavior:**
- **Không có application-level reconnection:** Player phải join room mới
- **TCP Resilience:** Brief network outages được OS TCP stack xử lý
- **UI Feedback:** Red "Connection Lost" banner khi PING/PONG timeout
- **Clean State:** Player disconnect được xử lý gracefully (adjust rounds, skip turn)

**Postcondition:** Player disconnected hoặc connection restored (nếu brief outage)

### 3.5. Use Case: Game End & Rankings

**Actor:** Tất cả người chơi  
**Precondition:** Tất cả rounds đã hoàn thành  

**Main Flow:**
1. Round cuối cùng kết thúc
2. Server tính toán:
   - Tìm player có điểm cao nhất (winner)
   - Sắp xếp players theo score giảm dần
3. Server broadcast `MSG_GAME_END` với:
   ```json
   {
     "players": [
       {"player_id": 1, "username": "Alice", "score": 850},
       {"player_id": 2, "username": "Bob", "score": 720},
       {"player_id": 3, "username": "Carol", "score": 650}
     ]
   }
   ```
4. Client hiển thị dialog "Game Ended" với rankings:
   - #1 Alice 👑 - 850 pts (Gold gradient)
   - #2 Bob - 720 pts (Silver gradient)
   - #3 Carol - 650 pts (Bronze gradient)
5. Player click "Return to Home" → quay về landing page

**Postcondition:** Game kết thúc, có thể chơi lại

### 3.6. Use Case: Host Start Game Early (Private Rooms)

**Actor:** Host của private room  
**Precondition:** Private room đã tạo, có 2+ players, game chưa bắt đầu  

**Main Flow:**
1. Host tạo hoặc join private room đầu tiên
2. Server đánh dấu host với `room->host_player_id`
3. Khi có 2+ players, countdown 15 giây bắt đầu
4. Host nhìn thấy button "Start Game" (chỉ host thấy)
5. Host click "Start Game" để bỏ qua countdown
6. Client gửi `MSG_HOST_START_GAME` (type 27)
7. Server validate:
   - `player_id == room->host_player_id`
   - `room->player_count >= 2`
   - `room->state == ROOM_WAITING`
8. Server dừng countdown và gọi `start_game(room)`
9. Server broadcast `MSG_GAME_START` cho tất cả players
10. Game bắt đầu ngay lập tức với round 1

**Alternative Flow - Invalid Start:**
- Nếu <2 players: Button greyed out, không gửi request
- Nếu không phải host: Button không hiển thị
- Nếu server reject: Gửi `MSG_ERROR` về client

**Postcondition:** Game bắt đầu ngay lập tức, bỏ qua countdown

### 3.7. Use Case: Host Kick Player (Private Rooms)

**Actor:** Host của private room  
**Precondition:** Private room có 2+ players, player cần kick đang trong room  

**Main Flow:**
1. Host hover mouse lên player card trong players list
2. Hiển thị kick icon (red X) ở góc player card
3. Host click kick icon
4. Client confirm dialog: "Kick [username] from room?"
5. Host confirm → Client gửi `MSG_HOST_KICK_PLAYER` (type 28) với `player_id`
6. Server validate:
   - Sender là host (`player_id == room->host_player_id`)
   - Target player tồn tại trong room
   - Target player ≠ host (không thể kick chính mình)
7. Server xóa player khỏi room:
   - Gọi `remove_player_from_room(room, target_player)`
   - Adjust drawer_idx nếu cần
   - Recalculate total_rounds
8. Server gửi `MSG_DISCONNECT` cho kicked player với reason "Kicked by host"
9. Server broadcast `MSG_PLAYER_LEAVE` cho players còn lại với reason "kicked"
10. Kicked player hiển thị dialog "Kicked from Room" với button "Return to Home"

**Alternative Flow - Invalid Kick:**
- Nếu không phải host: Kick icon không hiển thị
- Nếu kick chính mình: Server reject với `MSG_ERROR`
- Nếu target không trong room: Server reject

**Special Cases:**
- Nếu kick drawer đang vẽ → End round và chuyển lượt
- Nếu kick làm room còn <2 players → Cancel countdown (nếu đang countdown)

**Postcondition:** Player bị kick rời room, players còn lại tiếp tục game

---

## 4. CẤU TRÚC MÃ NGUỒN

### 4.1. Server Directory Structure

```
server/
├── main.c                          # Entry point, khởi tạo TCP server và timer
│
├── tcp/                           # TCP Server cho game logic
│   ├── tcp_server.c/.h           # TCP socket management với select()
│   ├── tcp_handler.c/.h          # Message handlers (32 types)
│   └── tcp_parser.c/.h           # JSON message parsing
│
├── game/                          # Game Logic
│   ├── game_logic.c/.h           # Core game mechanics
│   ├── matchmaking.c/.h          # Room management
│   └── stats.c/.h                # Player statistics persistence
│
└── utils/                         # Utilities
    ├── logger.c/.h               # JSON logging
    ├── timer.c/.h                # Timer thread
    └── json.c/.h                 # JSON utilities
```

### 4.2. Client C Library Directory Structure

```
client_c/
├── network.c/.h                   # TCP socket operations
│   ├── network_connect()         # Connect to server
│   ├── network_send_tcp()        # Send with length prefix
│   ├── network_recv_tcp()        # Receive with length prefix
│   └── network_disconnect()      # Clean disconnect
│
└── Compiled to:
    ├── libscribble_client.dylib  # macOS shared library
    └── libscribble_client.so     # Linux shared library
```

### 4.3. Pygame Client Directory Structure

```
client_pygame/
├── main.py                        # Main game loop và UI
│   ├── ScribbleGame class        # Game state management
│   ├── Button, InputBox          # UI components
│   ├── Canvas rendering          # Pygame drawing
│   └── Event handling            # Mouse, keyboard
│
├── network_wrapper.py             # ctypes wrapper cho C library
│   ├── NetworkClient class       # Python interface
│   ├── connect()                 # Bind C functions
│   ├── send_tcp()                # Send JSON messages
│   └── receive()                 # Non-blocking receive
│
├── protocol.py                    # Message types và constants
│   ├── MSG_TYPE enum             # 32 message types (0-31)
│   ├── Drawing types             # STROKE (100), CLEAR (101)
│   ├── COLORS palette            # 10 drawing colors
│   └── Helper functions          # Color conversion
│
├── resources.py                   # Asset management
│   └── ResourceManager           # Load fonts, icons
│
└── res/                          # Resources directory
    └── icon.png                  # Window icon
```

---

## 5. CÁC THÀNH PHẦN CHÍNH

### 5.1. Game Server Components

#### 5.1.1. TCP Server (`tcp/tcp_server.c`)
**Vai trò:** Quản lý game connections và per-client TCP sockets

**Chức năng chính:**
- Bind socket tại port 9090
- Maintain mảng players với TCP socket cho mỗi client
- Accept new connections và tạo Player struct
- Use `select()` để handle multiple clients
- Parse messages với 4-byte length prefix (big-endian)
- Route messages đến appropriate handlers

**Key Data Structures:**
```c
typedef struct {
    int fd;                    // TCP socket file descriptor
    char username[32];
    uint32_t player_id;
    int score;
    PlayerState state;
    bool is_drawing;
    bool has_guessed;
    bool has_drawn;            // Track if had turn
    char recv_buffer[4096];    // Partial message buffer
    int recv_buffer_len;
    
    // Stats tracking
    uint32_t correct_guesses_this_game;
    uint32_t rounds_drawn_this_game;
    uint64_t round_start_time;
} Player;
```

**Key Functions:**
```c
void* tcp_server_thread(void* arg);           // Main TCP thread
void handle_tcp_message(Player* player, ...); // Message router
void send_tcp_message(int fd, ...);           // Send with length prefix
void broadcast_to_room(Room* room, ...);      // Broadcast to all players
```

#### 5.1.2. TCP Handler (`tcp/tcp_handler.c`)
**Vai trò:** Process và route tất cả TCP messages

**Chức năng chính:**
- Handle 30+ message types
- Drawing messages (types 100-102): STROKE, CLEAR_CANVAS, UNDO
- Game flow messages: REGISTER, JOIN_ROOM, GAME_START, etc.
- Chat messages: CHAT, CHAT_BROADCAST
- State updates: TIMER_UPDATE, SCORE_UPDATE, etc.
- Broadcast messages to room members
- Filter messages (e.g., không gửi stroke về người vẽ)

**Note:** Drawing messages vẫn dùng tên `UDP_STROKE`, `UDP_CLEAR_CANVAS` cho historical reasons nhưng thực tế transmitted qua TCP.
#### 5.1.3. Game Logic (`game/game_logic.c`)
**Vai trò:** Core game mechanics và room management

**Key Data Structures:**
```c
typedef struct {
    uint32_t room_id;
    char room_code[16];              // 6-char code for private rooms
    Player* players[MAX_PLAYERS];    // Array of player pointers
    int player_count;
    uint32_t host_player_id;         // Host for private rooms (kick/start)
    RoomState state;                 // WAITING, PLAYING, ENDED
    int current_drawer_idx;
    char current_word[MAX_WORD_LEN];
    int round_number;
    int total_rounds;                // Dynamic: equals player_count
    uint64_t round_start_time;
    int time_remaining;
    Stroke strokes[MAX_STROKES];
    int stroke_count;
    bool is_private;
    uint64_t game_start_countdown;
    bool countdown_active;
} Room;
```

**Key Functions:**
```c
void start_game(Room* room);
  // - Set total_rounds = player_count
  // - Reset scores, has_drawn flags
  // - Start first round

void start_next_round(Room* room);
  // - Increment round_number
  // - Find next player who hasn't drawn
  // - Skip disconnected players
  // - Select random word
  // - Reset round timer
  // - Broadcast MSG_ROUND_START

void end_round(Room* room);
  // - Broadcast MSG_ROUND_END
  // - Call start_next_round()
  // - Or call end_game() if all done

void end_game(Room* room);
  // - Find winner
  // - Broadcast MSG_GAME_END with rankings
  // - Set state to ROOM_ENDED

int process_guess(Room* room, Player* player, const char* guess);
  // - Case-insensitive comparison
  // - Calculate score based on time
  // - Mark player.has_guessed = true
  // - Broadcast MSG_GUESS_CORRECT
  // - Check if all guessed → end round early

void update_timer(Room* room);
  // - Calculate elapsed time
  // - Update time_remaining
  // - Broadcast MSG_TIMER_UPDATE
  // - If time_remaining <= 0 → end_round()

void check_game_start_countdown(Room* room);
  // - Check if countdown active
  // - Broadcast MSG_COUNTDOWN_UPDATE every second
  // - After 15 seconds → start_game()

int add_player_to_room(Room* room, Player* player);
int remove_player_from_room(Room* room, Player* player);
  // - Adjust drawer_idx if current drawer leaves
  // - Recalculate total_rounds
  // - End round if drawer disconnects
```

#### 5.1.4. Matchmaking (`game/matchmaking.c`)
**Vai trò:** Auto matchmaking và room management

**Chức năng:**
- Maintain array of 100 rooms
- Auto matchmaking: tìm room có < MAX_PLAYERS
- Private room creation với 6-character code
- Join by room code
- Activate countdown khi 2nd player joins
- Prevent joining after round 1
- Iterate active rooms cho timer updates

**Key Functions:**
```c
int join_matchmaking(Player* player);
  // - Find best available room
  // - Add player to room
  // - If player_count == 2 → activate countdown

int create_private_room(Player* player);
  // - Generate unique 6-char code
  // - Initialize room
  // - Add creator

int join_private_room(Player* player, const char* code);
  // - Find room by code
  // - Check if joinable (round <= 1)
  // - Add player

void iterate_active_rooms(void (*callback)(Room*));
  // - Loop through ROOM_WAITING and ROOM_PLAYING
  // - Call callback for each active room
```

#### 5.1.5. Stats System (`game/stats.c`)
**Vai trò:** Player statistics tracking và CSV persistence

**Chức năng:**
- CSV file storage trong `player_stats.txt`
- Thread-safe operations với pthread_mutex
- Load stats on player register
- Update stats on game end
- Track fastest guess times

**Key Data Structures:**
```c
typedef struct {
    char username[32];
    uint32_t games_played;
    uint32_t games_won;
    uint64_t total_score;
    uint32_t total_correct_guesses;
    uint32_t total_rounds_drawn;
    uint64_t fastest_guess_ms;
    uint64_t last_played;
} PlayerStats;
```

**Key Functions:**
```c
int load_player_stats(const char* username, PlayerStats* stats);
  // - Read from CSV file
  // - Parse player stats
  // - Return 0 if found, -1 if new player

int save_player_stats(const PlayerStats* stats);
  // - Atomic write with temp file
  // - Thread-safe with mutex
  // - Update or append stats

void update_game_stats(Player* player, bool won);
  // - Increment games_played
  // - Increment games_won if won
  // - Add to total_score
  // - Update last_played timestamp

void update_fastest_guess(Player* player, uint64_t guess_time_ms);
  // - Compare with current fastest
  // - Update if faster
```

#### 5.1.6. Timer Thread (`utils/timer.c`)
**Vai trò:** 1-second tick timer cho game updates

**Chức năng:**
- Run trong separate thread
- Mỗi giây gọi callback:
  - `check_game_start_countdown()` cho countdown
  - `update_timer()` cho round timer
  - Broadcast `MSG_TIMER_UPDATE` và `MSG_COUNTDOWN_UPDATE`
- Thread-safe với mutex

**Implementation:**
```c
void* timer_thread(void* arg) {
    while (timer_running) {
        sleep(1);
        
        iterate_active_rooms([](Room* room) {
            check_game_start_countdown(room);
            update_timer(room);
        });
    }
}
```

### 5.2. C Networking Library (`client_c/`)

#### 5.2.1. Network Module (`network.c/.h`)
**Vai trò:** TCP socket wrapper compiled to shared library

**Chức năng chính:**
- TCP connection management
- 4-byte length prefix protocol (big-endian)
- Send/receive with proper framing
- Cross-platform: .dylib (macOS) / .so (Linux)

**Key Functions:**
```c
int network_connect(const char* host, int port);
int network_send_tcp(const char* msg, int len);
int network_recv_tcp(char* buffer, int buffer_size);
void network_disconnect();
```

### 5.3. Pygame Client (`client_pygame/`)

#### 5.3.1. Main Game (`main.py`)
**Vai trò:** UI rendering và game logic

**Components:**
- ScribbleGame class: Main game controller
- Button, InputBox: UI widgets
- Canvas: Pygame drawing surface
- State machine: LANDING → WAITING → PLAYING → ENDED

**Features:**
- 1200x700 window
- 60 FPS game loop
- Mouse/keyboard events
- Non-blocking network receive
- Drawing tools: 10 colors, variable brush size
- Chat system
- Players list sidebar
- Timer display

#### 5.3.2. Network Wrapper (`network_wrapper.py`)
**Vai trò:** Python ctypes bridge to C library

**Implementation:**
```python
class NetworkClient:
    def __init__(self):
        self.lib = ctypes.CDLL('libscribble_client.dylib')
        # Bind C functions
        
    def send_tcp(self, msg_type, data):
        message = {"type": msg_type, "data": data}
        json_str = json.dumps(message)
        return self.lib.network_send_tcp(...)
```

#### 5.3.3. Protocol (`protocol.py`)
**Vai trò:** Message types và constants

**Definitions:**
- MSG_TYPE: 30 TCP message types (0-29)
- Drawing types: STROKE (100), CLEAR_CANVAS (101), UNDO (102)
- COLORS: 10-color palette
- Helper functions

---

**Note:** Client proxy (WebSocket bridge) đã được deprecated. System hiện tại sử dụng direct TCP connection từ Pygame client đến server.

---

## 6. PROTOCOL VÀ MESSAGE FLOW

### 6.1. Message Types

#### TCP Messages (All communication via TCP port 9090)
```c
typedef enum {
    MSG_PING = 0,
    MSG_PONG = 1,
    MSG_REGISTER = 2,
    MSG_REGISTER_ACK = 3,
    MSG_JOIN_ROOM = 4,
    MSG_CREATE_ROOM = 5,
    MSG_ROOM_CREATED = 6,
    MSG_ROOM_JOINED = 7,
    MSG_ROOM_FULL = 8,
    MSG_ROOM_NOT_FOUND = 9,
    MSG_GAME_START = 10,
    MSG_YOUR_TURN = 11,
    MSG_WORD_TO_DRAW = 12,
    MSG_ROUND_START = 13,
    MSG_CHAT = 14,
    MSG_CHAT_BROADCAST = 15,
    MSG_GUESS_CORRECT = 16,
    MSG_GUESS_WRONG = 17,
    MSG_TIMER_UPDATE = 18,
    MSG_COUNTDOWN_UPDATE = 19,
    MSG_ROUND_END = 20,
    MSG_GAME_END = 21,
    MSG_PLAYER_JOIN = 22,
    MSG_PLAYER_LEAVE = 23,
    MSG_SCORE_UPDATE = 24,
    MSG_ERROR = 25,
    MSG_DISCONNECT = 26,
    MSG_HOST_START_GAME = 27,
    MSG_HOST_KICK_PLAYER = 28
} MessageType;

// Drawing Messages (also via TCP, historical naming)
typedef enum {
    UDP_STROKE = 100,        // Drawing stroke
    UDP_CLEAR_CANVAS = 101,  // Clear canvas
    UDP_UNDO = 102           // Undo last stroke (not implemented)
} UDPMessageType;  // Note: Naming is historical, actually sent via TCP
```

**Message Reference Table:**

| ID  | Name                 | Direction      | Purpose                                       |
|-----|----------------------|----------------|-----------------------------------------------|
| 0   | PING                 | Server→Client  | Keep-alive check                              |
| 1   | PONG                 | Client→Server  | Keep-alive response                           |
| 2   | REGISTER             | Client→Server  | Player registration with username             |
| 3   | REGISTER_ACK         | Server→Client  | Registration confirmation (player_id, token)  |
| 4   | JOIN_ROOM            | Client→Server  | Join room (auto or by code)                   |
| 5   | CREATE_ROOM          | Client→Server  | Create private room                           |
| 6   | ROOM_CREATED         | Server→Client  | Private room created with code                |
| 7   | ROOM_JOINED          | Server→Client  | Successfully joined room                      |
| 8   | ROOM_FULL            | Server→Client  | Room is full error                            |
| 9   | ROOM_NOT_FOUND       | Server→Client  | Invalid room code error                       |
| 10  | GAME_START           | Server→Client  | Game begins (round 1)                         |
| 11  | YOUR_TURN            | Server→Client  | You are now the drawer                        |
| 12  | WORD_TO_DRAW         | Server→Client  | Word for drawer only                          |
| 13  | ROUND_START          | Server→Client  | New round begins                              |
| 14  | CHAT                 | Client→Server  | Chat message or guess                         |
| 15  | CHAT_BROADCAST       | Server→Client  | Broadcast chat to room                        |
| 16  | GUESS_CORRECT        | Server→Client  | Player guessed correctly                      |
| 17  | GUESS_WRONG          | Server→Client  | Incorrect guess (deprecated, uses chat)       |
| 18  | TIMER_UPDATE         | Server→Client  | Round timer countdown                         |
| 19  | COUNTDOWN_UPDATE     | Server→Client  | Game start countdown (15s)                    |
| 20  | ROUND_END            | Server→Client  | Round finished, show word                     |
| 21  | GAME_END             | Server→Client  | Game finished, final scores                   |
| 22  | PLAYER_JOIN          | Server→Client  | New player joined room                        |
| 23  | PLAYER_LEAVE         | Server→Client  | Player left room                              |
| 24  | SCORE_UPDATE         | Server→Client  | Player score changed                          |
| 25  | ERROR                | Server→Client  | General error message                         |
| 26  | DISCONNECT           | Server→Client  | Server-initiated disconnect (kick)            |
| 27  | HOST_START_GAME      | Client→Server  | Host requests early game start                |
| 28  | HOST_KICK_PLAYER     | Client→Server  | Host kicks player from room                   |
| 100 | UDP_STROKE           | Client↔Server  | Drawing stroke data (via TCP)                 |
| 101 | UDP_CLEAR_CANVAS     | Client→Server  | Clear canvas command (via TCP)                |
| 102 | UDP_UNDO             | Client→Server  | Undo stroke (not implemented)                 |

### 6.2. Message Format

**TCP Message Structure (all messages):**
```
[4 bytes: length (big-endian)][JSON payload]
```

Example:
```
[0x00, 0x00, 0x00, 0x3A] {"type":2,"data":{"username":"Alice"}}
```

**Drawing Message (via TCP):**
```json
{
    "type": 100,
    "data": {
        "x1": 100.5,
        "y1": 150.2,
        "x2": 102.3,
        "y2": 152.1,
        "color": 0,      // Color palette index (0-9)
        "thickness": 5   // Brush size (2-20)
    }
}
```

### 6.3. Complete Game Flow

**1. Connection & Registration:**
```
Pygame → C Library → TCP → Server
{"type": 2, "data": {"username": "Alice"}}

Server → TCP → C Library → Pygame
{"type": 3, "data": {"player_id": 123, "username": "Alice"}}
```

**2. Join Matchmaking:**
```
Pygame → TCP → Server
{"type": 4, "data": {"room_id": 0}}

Server → TCP → Pygame
{"type": 7, "data": {"room_id": 1, "players": [...]}}
```

**3. Countdown (khi 2nd player joins):**
```
Server → All Players (every second via TCP)
{"type": 19, "data": {"countdown": 15}}
{"type": 19, "data": {"countdown": 14}}
...
{"type": 19, "data": {"countdown": 1}}
```

**4. Game Start:**
```
Server → All Players (via TCP)
{"type": 10, "data": {
    "round": 1,
    "total_rounds": 3,
    "players": [...],
    "word_mask": "_ _ _"
}}

Server → Drawer Only (via TCP)
{"type": 12, "data": {"word": "cat"}}
```

**5. Drawing Phase:**
```
Drawer → Server (via TCP, continuous)
{"type": 100, "data": {
    "x1": 100, "y1": 150,
    "x2": 102, "y2": 152,
    "color": 1, "thickness": 5
}}

Server → Other Players (broadcast via TCP)
{"type": 100, "data": {
    "x1": 100, "y1": 150,
    "x2": 102, "y2": 152,
    "color": 1, "thickness": 5,
    "player_id": 123
}}
```

**6. Guessing:**
```
Guesser → Server (via TCP)
{"type": 14, "data": {"message": "cat"}}

Server → All Players (if correct, via TCP)
{"type": 16, "data": {
    "player_id": 456,
    "username": "Bob",
    "score": 850
}}
```

**7. Round End & Next Round:**
```
Server → All Players (via TCP)
{"type": 20, "data": {"players": [...]}}

Server → All Players (via TCP)
{"type": 13, "data": {
    "round": 2,
    "total_rounds": 3,
    "word_mask": "_ _ _ _"
}}

Server → New Drawer (via TCP)
{"type": 12, "data": {"word": "bird"}}
```

**8. Game End:**
```
Server → All Players (via TCP)
{"type": 21, "data": {
    "players": [
        {"player_id": 123, "username": "Alice", "score": 950},
        {"player_id": 456, "username": "Bob", "score": 820},
        {"player_id": 789, "username": "Carol", "score": 710}
    ]
}}
```

**9. Host Controls (Private Rooms Only):**

*Early Game Start:*
```
Host → Server (via TCP)
{"type": 27, "data": {}}

Server validates:
- Player is host (player_id == room->host_player_id)
- Room has 2+ players
- Room in WAITING state

Server → All Players (via TCP)
{"type": 10, "data": {"round": 1, ...}}  // Game starts immediately
```

*Kick Player:*
```
Host → Server (via TCP)
{"type": 28, "data": {"player_id": 456}}

Server validates:
- Sender is host
- Target player exists in room
- Cannot kick self

Server → Kicked Player (via TCP)
{"type": 29, "data": {"reason": "Kicked by host"}}

Server → Other Players (via TCP)
{"type": 23, "data": {
    "player_id": 456,
    "username": "Bob",
    "reason": "kicked"
}}
```

---

## 7. TÍNH NĂNG ĐÃ HOÀN THÀNH

### 7.1. Core Game Mechanics ✅
- [x] **Turn-based gameplay:** Mỗi player vẽ 1 lượt
- [x] **Dynamic rounds:** Số round = số player (2-5 rounds)
- [x] **Word selection:** Random từ `words.txt` (2000+ từ)
- [x] **Scoring system:** Điểm dựa trên tốc độ đoán đúng
- [x] **Timer system:** 90 giây/round với countdown
- [x] **Round progression:** Tự động chuyển round khi hết giờ hoặc tất cả đoán xong

### 7.2. Multiplayer Features ✅
- [x] **Auto matchmaking:** Join room tự động khi < 5 players
- [x] **Private rooms:** Tạo room với 6-character code
- [x] **Join by code:** Tham gia private room
- [x] **2-5 players:** Game bắt đầu với tối thiểu 2 người
- [x] **15-second countdown:** Khi đủ 2 người, đếm ngược 15s
- [x] **No join after round 1:** Không cho join khi game đã bắt đầu

### 7.3. Drawing System ✅
- [x] **Real-time canvas sync:** Drawing đồng bộ cho tất cả players
- [x] **10-color palette:** Black, Red, Green, Blue, Yellow, Magenta, Cyan, Orange, Purple, Brown
- [x] **Brush size control:** 2-20px adjustable
- [x] **Clear canvas:** Drawer có thể xóa và vẽ lại
- [x] **Smooth strokes:** Line segments với lineCap: 'round'
- [x] **Touch support:** Vẽ được trên mobile devices

### 7.4. Chat & Guessing ✅
- [x] **Real-time chat:** Chat messages broadcast instantly
- [x] **Case-insensitive guess:** "Cat" = "cat" = "CAT"
- [x] **Guess feedback:**
  - Đúng: Green notification "Player X guessed correctly! 🎉"
  - Sai: Hiển thị như chat thường
- [x] **Guess once:** Mỗi player chỉ đoán đúng 1 lần/round
- [x] **Early round end:** Nếu tất cả đã đoán → end round sớm

### 7.5. UI/UX ✅
- [x] **Pygame rendering:** Hardware-accelerated graphics
- [x] **Responsive layout:** 1200x700 window
- [x] **Landing screen:** Play Now, Create Room, Join Room buttons
- [x] **Waiting screen:** Room code, countdown, players list
- [x] **Game screen:** Canvas, Players sidebar, Chat, Drawing tools
- [x] **Players sidebar:** Names, scores, online status, drawing indicator
- [x] **Room info:** Room code display, Round X/Y counter
- [x] **Timer display:** Countdown timer màu đỏ khi < 10s
- [x] **Word display:** Masked word với underscores
- [x] **Drawer sees word:** Full word displayed at top
- [x] **Status messages:** Info/success/error notifications
- [x] **Game end screen:** Rankings với crown 👑 cho winner
- [x] **Return to home button:** Quay về landing sau game
- [x] **Smooth drawing:** Line caps và circles cho smooth strokes
- [x] **Color palette:** 10 colors với visual selection feedback
- [x] **Brush size slider:** 2-20px adjustable thickness

### 7.6. Server Infrastructure ✅
- [x] **TCP server:** Game logic và communication (port 9090)
- [x] **Multi-threading:** TCP server thread + Timer thread
- [x] **Per-client sockets:** Mỗi player có TCP socket riêng
- [x] **Message broadcasting:** Efficient room broadcast
- [x] **Connection handling:** Clean disconnect detection
- [x] **select() multiplexing:** Handle multiple clients efficiently

### 7.7. Client Library ✅
- [x] **C networking library:** Compiled shared library
- [x] **TCP socket management:** Connect, send, receive, disconnect
- [x] **Message framing:** 4-byte length prefix protocol
- [x] **Big-endian byte order:** Network byte order compliance
- [x] **Cross-platform build:** .dylib (macOS) / .so (Linux)
- [x] **ctypes integration:** Python-C bridge via ctypes
- [x] **Non-blocking receive:** Timeout-based receive
- [x] **Error handling:** Proper return codes

### 7.8. Cross-Platform ✅
- [x] **macOS support:** Compile và run native
- [x] **Linux support:** Compatible build system
- [x] **Python 3.x:** Compatible với Python 3.8+
- [x] **Pygame support:** Works với Pygame 2.x

### 7.9. Player Management ✅
- [x] **Player registration:** Assign unique player_id
- [x] **Username tracking:** Display names in UI
- [x] **Score tracking:** Persistent trong game session
- [x] **Drawing state:** Track is_drawing, has_guessed, has_drawn
- [x] **Player leave handling:**
  - Adjust drawer index
  - Recalculate total_rounds
  - Skip disconnected players
  - End round nếu drawer leaves

### 7.10. Logging ✅
- [x] **Event logging:** JSON format logs
- [x] **Room events:** Created, started, ended
- [x] **Player events:** Joined, left, score updates
- [x] **Round events:** Started, ended, word selected
- [x] **Guess logging:** Correct/wrong guesses
- [x] **Stroke logging:** Drawing actions

### 7.11. Persistent Player Stats ✅
- [x] **Stats file system:** CSV format in `player_stats.txt`
- [x] **Tracked metrics:**
  - Games played and won
  - Total score accumulated
  - Correct guesses count
  - Rounds drawn count
  - Fastest guess time (milliseconds)
  - Last played timestamp
- [x] **Auto save:** Stats updated after each game ends
- [x] **Load on register:** Player stats loaded when connecting
- [x] **Thread-safe:** Mutex-protected file operations with pthread_mutex
- [x] **Leaderboard support:** Get top N players by total score
- [x] **CSV format:**
  ```
  username,games_played,games_won,total_score,correct_guesses,rounds_drawn,fastest_guess_ms,last_played
  Alice,10,3,8500,45,10,1250,1735200000000
  Bob,8,2,6200,38,8,1580,1735199800000
  ```
- [x] **Integration:**
  - `load_player_stats()` called on MSG_REGISTER
  - `update_game_stats()` called on MSG_GAME_END
  - `update_fastest_guess()` called on MSG_GUESS_CORRECT
  - Stats added to Player struct: `correct_guesses_this_game`, `rounds_drawn_this_game`, `round_start_time`
- [x] **Atomic updates:** Temp file + rename for crash safety

### 7.12. Connection Monitoring & TCP Resilience ✅
- [x] **PING/PONG System:** Client-initiated keepalive every 5 seconds
- [x] **Connection health:** Server responds with PONG, client tracks timeout
- [x] **15-second timeout:** Connection lost detected after no PONG for 15s
- [x] **Connection lost UI:** Red warning banner displayed when disconnected
- [x] **TCP resilience:** Brief network outages handled by OS-level TCP buffering
- [x] **Online indicators:** Players see others go offline/online via disconnect messages
- [x] **Simplified architecture:** No application-level reconnection attempts
- [x] **Clean disconnects:** Server notifies room when player leaves
- [x] **Messages:** MSG_PING, MSG_PONG, MSG_DISCONNECT, MSG_PLAYER_LEAVE

### 7.13. Host Controls for Private Rooms ✅
- [x] **Host assignment:** First player to create/join room becomes host
- [x] **Early game start:**
  - Host can start game with 2+ players (bypass 15s countdown)
  - "Start Game" button visible only to host in waiting state
  - Button greyed out when <2 players
  - Server validates host permission and player count
- [x] **Kick player:**
  - Hover over player card shows kick icon (red X)
  - Only host can see kick icons (not on self)
  - Click to remove player from room
  - Kicked player receives disconnect message with reason
  - Kicked player shown "Kicked from Room" screen with return button
  - Server validates host permission
- [x] **Message types:**
  - MSG_HOST_START_GAME (27): Host requests early start
  - MSG_HOST_KICK_PLAYER (28): Host removes player
- [x] **Permission system:**
  - Room struct tracks host_player_id
  - Server validates all host actions
  - Prevents self-kick and unauthorized actions

### 7.14. Resource Management ✅
- [x] **Programmatic icon generation:**
  - Send icon (paper plane shape)
  - Kick icon (red circle with white X)
  - Drawing icon (pencil shape)
  - Clear icon (eraser/trash)
- [x] **Optional resource loading:**
  - Background textures (bg_wood) - used in main UI
  - Logo images - graceful fallback if missing
- [x] **Fallback system:**
  - Creates icons if files not found
  - Pygame surface generation with basic shapes
  - No critical failures from missing assets






## 8. KẾT LUẬN

### 8.1. Thành tựu đạt được

Dự án Scribble đã hoàn thành các mục tiêu chính:

✅ **Architecture Design**
- Multi-threaded C server với TCP protocol
- Pygame client với C networking library
- Clean separation of concerns
- CSV-based persistence layer

✅ **Core Gameplay**
- Turn-based drawing và guessing
- Real-time canvas synchronization
- Dynamic rounds (2-5 players)
- Scoring system với time-based points
- Auto matchmaking và private rooms
- Host controls (early start, kick players)

✅ **Network Programming**
- Per-client TCP connections
- Message broadcasting với proper routing
- Cross-platform compatibility (macOS, Linux, WSL)
- JSON message format
- Session-based reconnection system

✅ **User Experience**
- Pygame native UI với hardware acceleration
- 10-color palette với visual feedback
- Real-time timer và countdown
- Game end rankings với winner display
- Smooth drawing với proper stroke rendering
- Resource fallback system

✅ **Data Persistence**
- Player statistics tracking (CSV)
- Session tokens for reconnection
- Thread-safe file operations
- Atomic updates for data integrity

### 8.2. Kỹ năng học được

**Network Programming:**
- Berkeley sockets (TCP)
- Multi-threading với pthreads
- Thread synchronization (mutex, condition variables)
- Message serialization/deserialization
- Connection monitoring (PING/PONG keepalive)
- TCP resilience and reliability

**System Design:**
- Client-server architecture
- State management
- Thread-safe data structures
- CSV-based persistence
- Host permission system

**C Programming:**
- Memory management
- Pointer manipulation
- Cross-platform development (.dylib/.so)
- Build systems (Makefile)
- Shared library compilation
- ctypes integration
- File I/O with atomic operations

**Python Programming:**
- Pygame framework
- Event-driven programming
- ctypes FFI (Foreign Function Interface)
- JSON serialization
- Resource management
- Callback patterns

**Game Development:**
- Game loop design (60 FPS)
- State machine implementation
- Real-time rendering
- Input handling
- UI component design
- Canvas drawing systems

### 8.3. Hạn chế và cải tiến

**Hạn chế hiện tại:**
1. Chưa có latency-based matchmaking
2. No automatic reconnection (relies on TCP resilience only)
3. Stats UI chưa được hiển thị trong game
4. Memory leaks potential chưa fully test
5. Performance chưa optimize cho scale lớn
6. Host transfer khi host leaves chưa được implement

**Hướng phát triển:**
1. **Architecture:**
   - Multi-threaded Pygame client
   - Async network I/O
   - Event-driven message handling
   - Separate render thread

2. **Scalability:**
   - Implement room sharding
   - Load balancing với multiple servers
   - Redis cho shared state
   - Database persistence

3. **Security:**
   - Authentication system
   - Rate limiting
   - Input validation
   - Secure session tokens

4. **Features:**
   - Application-level reconnection system (if needed for production)
   - In-game stats display
   - Player profiles with stats history
   - Global leaderboards (top scores, fastest guesses)
   - Host transfer on host disconnect
   - Spectator mode
   - Undo drawing
   - Hint system
   - Advanced scoring (drawer points)
   - Custom word lists
   - Drawing time limit options
   - Room settings (rounds, timer, difficulty)

5. **Performance:**
   - Stroke batching
   - Binary protocol (if needed)
   - cJSON library
   - Connection pooling
   - Optimized canvas rendering

6. **DevOps:**
   - Docker containerization
   - CI/CD pipeline
   - Monitoring (logs analysis)
   - Automated testing
   - Monitoring (Prometheus)
   - Logging aggregation


## PHỤ LỤC

### A. Cấu trúc File quan trọng

**server/protocol.h** - Core data structures và message types  
**server/game/game_logic.c** - Game mechanics  
**server/game/stats.c/.h** - Player statistics system  
**server/tcp/tcp_handler.c** - Message handlers  
**server/tcp/tcp_server.c** - TCP server với select()  
**client_c/network.c** - C networking library  
**client_pygame/main.py** - Pygame game client  
**client_pygame/network_wrapper.py** - ctypes wrapper  
**client_pygame/protocol.py** - Message type definitions  
**client_pygame/resources.py** - Resource manager with fallback icons  
**player_stats.txt** - CSV persistence file for player statistics

### B. Data Files

**words.txt** - Word dictionary (2000+ words)  
**player_stats.txt** - Player statistics (CSV format)  
**logs/events.log** - Game events logging (JSON format)

### C. Build và Run

```bash
# Build all
make clean && make all

# Run server
make run-server
# Or: ./build/scribble_server

# Run client (in separate terminal)
make run-client
# Or: python3 client_pygame/main.py

# Run client with custom host/port
python3 client_pygame/main.py --host 192.168.1.2 --port 9090

# Stop server
Ctrl+C hoặc make stop
```


### D. Deprecated Components

**deprecated/udp/** - UDP implementation files (removed Dec 2024)
- `udp_server.c/.h` - UDP server (41-byte binary protocol)
- `udp_broadcast.c/.h` - UDP broadcasting logic
- **Reason:** Reverted to TCP-only for simplicity

**deprecated/client_proxy/** - WebSocket proxy (removed Dec 2024)
- Multi-threaded WebSocket bridge
- **Reason:** Migrated to direct Pygame client

**deprecated/server_http/** - HTTP server (removed Dec 2024)
- Static file serving
- **Reason:** No longer needed without Web UI

**deprecated/webui/** - Web frontend (removed Dec 2024)
- HTML/CSS/JavaScript client
- **Reason:** Replaced with Pygame client

---

**End of Technical Report**
