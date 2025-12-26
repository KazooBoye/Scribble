# BÁO CÁO KỸ THUẬT DỰ ÁN SCRIBBLE
## Multiplayer Drawing & Guessing Game

**Ngày báo cáo:** 26/12/2025  
**Môn học:** Lập trình mạng (Network Programming)  
**Người thực hiện:** Cao Duc Anh

---

## 📋 MỤC LỤC

1. [Tổng quan dự án](#1-tổng-quan-dự-án)
2. [Kiến trúc hệ thống](#2-kiến-trúc-hệ-thống)
3. [Use Cases](#3-use-cases)
4. [Cấu trúc mã nguồn](#4-cấu-trúc-mã-nguồn)
5. [Các thành phần chính](#5-các-thành-phần-chính)
6. [Protocol và Message Flow](#6-protocol-và-message-flow)
7. [Tính năng đã hoàn thành](#7-tính-năng-đã-hoàn-thành)
8. [Tính năng chưa hoàn thành](#8-tính-năng-chưa-hoàn-thành)
9. [Vấn đề và Giải pháp](#9-vấn-đề-và-giải-pháp)
10. [Kết luận](#10-kết-luận)

---

## 1. TỔNG QUAN DỰ ÁN

### 1.1. Mô tả dự án
Scribble là một trò chơi multiplayer real-time tương tự skribbl.io, được xây dựng với C backend server kết hợp với Pygame client. Server sử dụng C thuần với POSIX threads và Berkeley sockets, trong khi client sử dụng Python/Pygame với C networking library thông qua ctypes.

### 1.2. Mục tiêu
- Xây dựng game server xử lý logic game và quản lý phòng chơi
- Phát triển Pygame client với C networking library
- Triển khai hệ thống matchmaking thông minh
- Đảm bảo đồng bộ real-time cho drawing và chat qua TCP
- Xử lý reconnection và persistence
- Tạo UI trực quan với Pygame rendering

### Server**: C11, POSIX threads, Berkeley sockets
- **Client**: Python 3.x, Pygame, ctypes
- **Networking Library**: C (compiled to .dylib/.so)
- **Protocol**: TCP with 4-byte length prefix
- **Data Format**: JSON
- **Cross-platform**: macOS, Linux
- **Cross-platform**: macOS, Linux/WSL

---

## 2. KIẾN TRÚC HỆ THỐNG

### 2.1. Sơ đồ tổng quan

```
┌─────────────────────────────────────────────────────────────┐
│              PYGAME CLIENTS (Python + Pygame)               │
│         Pygame UI + Canvas + ctypes → C Library             │
└──────────────────┬──────────────────────────────────────────┘
                   │ ctypes call
                   ▼
┌─────────────────────────────────────────────────────────────┐
│           C NETWORKING LIBRARY (libscribble_client)         │
│              network.c - Berkeley Sockets                   │
│              - TCP socket management                        │
│              - Message framing (length prefix)              │
│              - Compiled to .dylib (macOS) / .so (Linux)     │
│              - Called via Python ctypes                     │
└──────────────────┬──────────────────────────────────────────┘
                   │ TCP (Port 9090)
                   ▼
┌─────────────────────────────────────────────────────────────┐
│                      GAME SERVER (C Program)                │
├─────────────────┬──────────────┬──────────────┬─────────────┤
│  TCP Server     │  Game Engine │  Matchmaking │  Timer      │
│  (Port 9090)    │              │              │  Thread     │
│  - Per-client   │  - Rooms     │  - Auto      │  - 1Hz      │
│    sockets      │  - Rounds    │    match     │    tick     │
│  - Select()     │  - Scoring   │  - Private   │  - Round    │
│  - Broadcast    │  - Drawing   │    rooms     │    timer    │
│  - JSON msg     │    state     │  - Join code │  - Updates  │
└─────────────────┴──────────────┴──────────────┴─────────────┘
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

### 3.4. Use Case: Player Disconnect & Reconnect

**Actor:** Người chơi bị mất kết nối  
**Precondition:** Game đang chơi, kết nối bị gián đoạn  

**Main Flow - Disconnect:**
1. Client proxy phát hiện connection lost
2. Gửi `MSG_DISCONNECT` hoặc timeout
3. Server lưu player state với `session_token`
4. Server broadcast `MSG_PLAYER_LEAVE` cho players còn lại
5. Server giữ player state trong 5 phút
6. Game tiếp tục với players còn lại

**Main Flow - Reconnect:**
1. Player mở lại browser và connect
2. Client có `session_token` đã lưu trong localStorage
3. Gửi `MSG_RECONNECT_REQUEST` với token
4. Server validate token và restore player state
5. Server gửi `MSG_RECONNECT_SUCCESS` với full game state
6. Client restore UI: canvas, scores, timer, turn
7. Player tiếp tục game từ nơi đã dừng

**Postcondition:** Player trở lại game với state đúng

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

---

## 4. CẤU TRÚC MÃ NGUỒN

### 4.1. Server Directory Structure

```
server/
├── main.c                          # Entry point, khởi tạo TCP server và timer
│
├── tcp/                           # TCP Server cho game logic
│   ├── tcp_server.c/.h           # TCP socket management với select()
│   ├── tcp_handler.c/.h          # Message handlers (30+ types)
│   └── tcp_parser.c/.h           # JSON message parsing
│
├── game/                          # Game Logic
│   ├── game_logic.c/.h           # Core game mechanics
│   ├── matchmaking.c/.h          # Room management
│   └── reconnection.c/.h         # Reconnection handling
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
│   ├── MSG_TYPE enum             # 30 TCP message types
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
    char session_token[64];    // For reconnection
    char recv_buffer[4096];    // Partial message buffer
    int recv_buffer_len;
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

#### 5.1.5. Timer Thread (`utils/timer.c`)
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
    MSG_RECONNECT_REQUEST = 25,
    MSG_RECONNECT_SUCCESS = 26,
    MSG_RECONNECT_FAIL = 27,
    MSG_ERROR = 28,
    MSG_DISCONNECT = 29
} MessageType;

// Drawing Messages (also via TCP, historical naming)
typedef enum {
    UDP_STROKE = 100,        // Drawing stroke
    UDP_CLEAR_CANVAS = 101,  // Clear canvas
    UDP_UNDO = 102           // Undo last stroke (not implemented)
} UDPMessageType;  // Note: Naming is historical, actually sent via TCP
```

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
{"type": 3, "data": {"player_id": 123, "session_token": "..."}}
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
- [x] **Stats file system:** CSV format in `server/data/player_stats.txt`
- [x] **Tracked metrics:**
  - Games played and won
  - Total score accumulated
  - Correct guesses count
  - Rounds drawn count
  - Fastest guess time (milliseconds)
  - Last played timestamp
- [x] **Auto save:** Stats updated after each game ends
- [x] **Load on register:** Player stats loaded when connecting
- [x] **Thread-safe:** Mutex-protected file operations
- [x] **Leaderboard support:** Get top N players by total score

### 7.12. Reconnection System ✅
- [x] **Session tokens:** Generated on player register
- [x] **State preservation:** Player/room state saved on disconnect (5min timeout)
- [x] **Auto-reconnect:** Client detects disconnection and auto-retries
- [x] **State restoration:** Full game state restored on successful reconnect
- [x] **Messages:** MSG_RECONNECT_REQUEST/SUCCESS/FAIL
- [x] **Server-side:**
  - Save player state to memory on disconnect
  - Cleanup expired states (>5 minutes)
  - Restore player to room on reconnect
- [x] **Client-side:**
  - Store session token locally
  - Detect connection loss
  - Auto-retry with exponential backoff
  - UI feedback for reconnection status

---

## 8. TÍNH NĂNG CHƯA HOÀN THÀNH

### 8.1. Matchmaking by Latency ⚠️
**Mô tả ban đầu:** Ghép người chơi có ping tương tự nhau để đảm bảo fair play

**Trạng thái:**
- ❌ Chưa implement RTT measurement
- ❌ Chưa có ping-based matching algorithm
- ✅ Có basic auto matchmaking (first available room)

**Lý do chưa hoàn thành:**
- Focus vào core gameplay trước
- Cần thêm ping measurement mechanism
- Với số lượng player test nhỏ, latency không critical

**TODO để hoàn thành:**
```c
// 1. Add ping measurement
void measure_player_rtt(Player* player) {
    send_tcp_message(player->fd, MSG_PING, "{}");
    uint64_t start = get_current_time_ms();
    // Wait for MSG_PONG
    uint64_t end = get_current_time_ms();
    player->rtt = end - start;
}

// 2. Modify matchmaking
Room* find_best_room_by_latency(Player* player) {
    Room* best = NULL;
    uint64_t min_rtt_diff = UINT64_MAX;
    
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].state == ROOM_WAITING) {
            uint64_t avg_rtt = calculate_average_rtt(&rooms[i]);
            uint64_t diff = abs(avg_rtt - player->rtt);
            if (diff < min_rtt_diff) {
                min_rtt_diff = diff;
                best = &rooms[i];
            }
        }
    }
    return best;
}
```

### 8.2. Full Reconnection System ⚠️
**Mô tả ban đầu:** Player có thể reconnect và tiếp tục game từ đúng điểm đã disconnect

**Trạng thái:**
- ✅ Session token generation
- ✅ Player state preservation (5 phút)
- ❌ Canvas state restore chưa hoàn chỉnh
- ❌ Client-side reconnection UI chưa polish

**Vấn đề:**
- Canvas strokes không được lưu persistent
- Client cần redraw toàn bộ canvas sau reconnect
- Reconnection dialog hiển thị nhưng UX chưa tốt

**TODO để hoàn thành:**
```c
// Server-side: Save strokes
typedef struct {
    uint32_t player_id;
    Stroke strokes[MAX_STROKES];
    int stroke_count;
    // ... other state
} SavedPlayerState;

void save_player_state(Player* player, Room* room) {
    SavedPlayerState* state = malloc(sizeof(SavedPlayerState));
    state->player_id = player->player_id;
    state->stroke_count = room->stroke_count;
    memcpy(state->strokes, room->strokes, 
           sizeof(Stroke) * room->stroke_count);
    // Save to hash map with session_token as key
}

void restore_player_state(Player* player, SavedPlayerState* state) {
    // Send all strokes to player
    for (int i = 0; i < state->stroke_count; i++) {
        send_tcp_message(player->fd, UDP_STROKE, 
                        serialize_stroke(&state->strokes[i]));
    }
}
```

```javascript
// Client-side: Better reconnection UX
async handleReconnect() {
    const token = localStorage.getItem('session_token');
    if (!token) return;
    
    try {
        await this.ws.connect('ws://localhost:8081');
        this.ws.send(MSG_TYPE.RECONNECT_REQUEST, { token });
        
        // Show loading dialog
        this.showReconnectDialog('Restoring game state...');
    } catch (error) {
        this.showReconnectDialog('Reconnection failed. Starting new session.');
        localStorage.removeItem('session_token');
    }
}
```

### 8.3. Private Room Password Protection ❌
**Mô tả ban đầu:** Private rooms có thể có password để tăng security

**Trạng thái:**
- ❌ Chưa implement
- ✅ Private rooms hoạt động với 6-char code

**Lý do chưa hoàn thành:**
- 6-char random code đã đủ secure cho use case thông thường
- Password thêm friction vào UX
- Priority thấp hơn core features

**TODO để hoàn thành:**
```c
// Add to Room struct
typedef struct {
    // ... existing fields
    char password[64];      // SHA256 hash of password
    bool has_password;
} Room;

// Hash password
void hash_password(const char* plain, char* hash_out) {
    // Use SHA256
}

// Verify password
bool verify_password(Room* room, const char* plain) {
    char hash[64];
    hash_password(plain, hash);
    return strcmp(room->password, hash) == 0;
}
```

### 8.4. Spectator Mode ❌
**Mô tả:** Cho phép người khác xem game đang chơi mà không tham gia

**Trạng thái:**
- ❌ Chưa implement

**Lý do chưa hoàn thành:**
- Cần separate spectator state
- Complicate matchmaking logic
- Priority thấp

### 8.5. Undo Drawing ❌
**Mô tả:** Drawer có thể undo stroke cuối cùng

**Trạng thái:**
- ✅ Message type `UDP_UNDO` đã define
- ❌ Logic chưa implement

**Lý do chưa hoàn thành:**
- Clear canvas đã đủ cho basic use case
- Cần track stroke history phức tạp

**TODO để hoàn thành:**
```javascript
// Client
class DrawingCanvas {
    constructor() {
        this.strokeHistory = [];
    }
    
    undo() {
        if (this.strokeHistory.length > 0) {
            this.strokeHistory.pop();
            this.redraw();
            this.ws.send(UDP_TYPE.UNDO, {});
        }
    }
    
    redraw() {
        this.clear();
        for (const stroke of this.strokeHistory) {
            this.drawStroke(stroke);
        }
    }
}
```

### 8.6. Advanced Scoring System ❌
**Mô tả ban đầu:** Điểm thưởng cho drawer khi nhiều người đoán đúng

**Trạng thái:**
- ✅ Basic scoring: Guesser nhận điểm dựa trên thời gian
- ❌ Drawer không nhận điểm

**TODO để hoàn thành:**
```c
int process_guess(Room* room, Player* player, const char* guess) {
    // ... existing guess logic
    
    if (correct) {
        // Award points to guesser
        player->score += points;
        
        // Award points to drawer
        Player* drawer = room->players[room->current_drawer_idx];
        drawer->score += 5;  // Fixed points per correct guess
        
        // Broadcast both score updates
    }
}
```

### 8.7. Hint System ❌
**Mô tả:** Show hints sau một khoảng thời gian (ví dụ: reveal 1 chữ cái)

**Trạng thái:**
- ❌ Chưa implement

**TODO để hoàn thành:**
```c
void update_timer(Room* room) {
    // ... existing timer logic
    
    // Reveal hint at 60s, 30s remaining
    if (room->time_remaining == 60 || room->time_remaining == 30) {
        reveal_hint(room);
    }
}

void reveal_hint(Room* room) {
    // Find a hidden letter to reveal
    char word_mask[MAX_WORD_LEN];
    create_word_mask_with_hint(room->current_word, word_mask, 
                                room->time_remaining);
    
    // Broadcast updated mask
    char msg[256];
    snprintf(msg, sizeof(msg), "{\"word_mask\":\"%s\"}", word_mask);
    broadcast_to_room(room, MSG_HINT_UPDATE, msg, NULL);
}
```

---

## 9. VẤN ĐỀ VÀ GIẢI PHÁP

### 9.1. Migration từ Web UI sang Pygame Client ✅ COMPLETED

**Context ban đầu:**
- Server có HTTP server + WebSocket proxy + UDP server
- Client là Web UI với HTML/CSS/JavaScript
- Architecture phức tạp: Browser → WebSocket → Proxy → TCP/UDP → Server

**Vấn đề:**
1. **WebSocket proxy** thêm complexity không cần thiết
2. **HTTP server** chỉ serve static files
3. **Web Canvas API** có limitations về performance
4. **Browser limitations:** Không thể dùng UDP trực tiếp
5. **4-thread proxy** overkill cho simple game

**Giải pháp:**
- Migrate toàn bộ sang Pygame client với direct TCP connection
- Loại bỏ HTTP server, WebSocket proxy
- Client library trong C compiled thành shared library
- Python ctypes bridge cho Pygame

**Kết quả:**
✅ Architecture đơn giản hơn rất nhiều
✅ Direct TCP connection, không cần proxy
✅ Native performance với Pygame rendering
✅ Code maintainability tốt hơn
✅ Easier deployment (không cần web server)

### 9.2. UDP Implementation và Complete Reversion ✅ COMPLETED

#### Timeline của UDP Feature

**Phase 1: Initial UDP Implementation (Dec 2024)**
**Mô tả:** Implement UDP protocol để broadcast drawing strokes cho performance

**Implementation:**
- Server có UDP server thread tại port 9091
- Binary protocol: 41-byte packets
  ```c
  struct UDPStroke {
      uint8_t type;           // 1 byte: message type
      uint32_t room_id;       // 4 bytes: room identifier
      uint32_t stroke_id;     // 4 bytes: stroke sequence
      float x1, y1, x2, y2;   // 16 bytes: coordinates
      uint32_t color;         // 4 bytes: color value
      uint32_t thickness;     // 4 bytes: brush size
      uint64_t timestamp;     // 8 bytes: timing
  };  // Total: 41 bytes
  ```
- Client gửi strokes qua UDP socket
- Server nhận và broadcast via UDP

**Problems Encountered:**
1. **Player identification issue:** UDP packets không có connection state, phải lookup player bằng IP address
2. **Hybrid broadcast:** Nhận UDP nhưng phải broadcast lại qua TCP vì client logic
3. **Complexity explosion:** Maintain cả TCP và UDP sockets, sync state giữa 2 protocols
4. **Observer không thấy canvas:** UDP-to-TCP conversion có bugs
5. **Binary packing overhead:** Client phải pack/unpack binary structs

**Phase 2: UDP-to-TCP Hybrid (mid-Dec 2024)**
**Attempt:** Keep UDP receive nhưng broadcast via TCP

**Implementation:**
```c
// Server nhận UDP packet
void handle_udp_packet(const char* packet, struct sockaddr_in* client_addr) {
    // Deserialize binary packet
    Stroke stroke = deserialize_udp_stroke(packet);
    
    // Find player by IP address
    Player* player = find_player_by_ip(client_addr->sin_addr.s_addr);
    
    // Broadcast via TCP to room members
    broadcast_to_room(player->room, UDP_STROKE, serialize_json(stroke), player);
}
```

**Still problematic:**
- IP-based player lookup unreliable (NAT, proxies)
- UDP packets có thể arrive out-of-order
- Error handling phức tạp
- Debugging nightmare

**Phase 3: Complete Reversion to TCP (late Dec 2024)** ✅

**Decision:** Abandon UDP hoàn toàn, chuyển strokes về TCP với JSON

**Rationale:**
1. **Simplicity > Performance:** TCP overhead không đáng kể cho game này
2. **Reliability:** TCP đảm bảo delivery và ordering
3. **Single protocol:** Easier to debug và maintain
4. **JSON consistency:** Không cần binary packing/unpacking

**Implementation Steps:**
1. ✅ Removed UDP server initialization từ `server/main.c`
2. ✅ Updated `tcp/tcp_handler.c` để handle stroke messages (type 100)
3. ✅ Removed UDP socket từ `client_c/network.c`
4. ✅ Removed `network_send_udp()` function
5. ✅ Updated Python client để gửi strokes via TCP
6. ✅ Changed `send_udp()` → `send_tcp()` calls
7. ✅ Removed binary packing code (80+ lines)
8. ✅ Updated Makefile để không compile UDP sources
9. ✅ Moved UDP files to `deprecated/udp/`

**Code Changes:**
```python
# Before (Binary UDP):
def _pack_udp_stroke(self, stroke_id, x1, y1, x2, y2, color, thickness):
    return struct.pack(
        '!BIIffffIIQ',  # 41 bytes
        100, self.room_id, stroke_id,
        x1, y1, x2, y2,
        color, thickness, int(time.time() * 1000)
    )

# After (JSON TCP):
def send_stroke(self, x1, y1, x2, y2, color, thickness):
    self.send_tcp(MSG_TYPE.STROKE, {
        "x1": x1, "y1": y1,
        "x2": x2, "y2": y2,
        "color": color,
        "thickness": thickness
    })
```

**Results:**
✅ **Simplified architecture:** TCP-only communication
✅ **Working canvas sync:** Observer nhìn thấy drawing real-time
✅ **Maintainable code:** JSON messages dễ debug
✅ **No performance issues:** TCP đủ nhanh cho use case này
✅ **Code reduction:** Removed 200+ lines of UDP code

**Lessons Learned:**
1. **KISS principle:** Keep It Simple, Stupid - đừng over-engineer
2. **Premature optimization:** UDP "cho performance" nhưng không cần thiết
3. **Complexity cost:** Hybrid protocols tốn effort maintain hơn performance gain
4. **Debug difficulty:** Binary protocols khó debug hơn JSON nhiều
5. **TCP is good enough:** Cho real-time game scale nhỏ, TCP performance OK

**Historical Note:**
Message types 100-102 vẫn giữ prefix `UDP_` trong code (ví dụ `UDP_STROKE`) vì lý do historical. Chúng thực tế được transmitted qua TCP.

### 9.3. Race Condition Issues

#### 9.3.1. Timer Thread Race Condition ⚠️
**Vấn đề:**
- Timer thread và TCP handler thread cùng access room state
- Không có mutex protection
- Có thể xảy ra:
  - Timer update trong lúc processing guess
  - Round end trigger trong lúc broadcasting stroke
  - Corrupted room state

**Ví dụ:**
```c
// Thread 1 (Timer)
void update_timer(Room* room) {
    room->time_remaining--;  // ← Race here
    if (room->time_remaining <= 0) {
        end_round(room);     // ← Modifies room state
    }
}

// Thread 2 (TCP Handler)
int process_guess(Room* room, ...) {
    if (room->time_remaining > 0) {  // ← Race here
        // ... process guess
        room->players[i]->score += points;  // ← Modifies player state
    }
}
```

**Giải pháp đã implement:**
- Sử dụng `pthread_mutex_lock/unlock` trong matchmaking
- Tuy nhiên **chưa protect toàn bộ room operations**

**Giải pháp hoàn chỉnh cần:**
```c
// Add mutex to Room struct
typedef struct {
    // ... existing fields
    pthread_mutex_t room_mutex;
} Room;

// Protect all room operations
void update_timer(Room* room) {
    pthread_mutex_lock(&room->room_mutex);
    room->time_remaining--;
    if (room->time_remaining <= 0) {
        end_round(room);
    }
    pthread_mutex_unlock(&room->room_mutex);
}

int process_guess(Room* room, ...) {
    pthread_mutex_lock(&room->room_mutex);
    // ... safe access
    pthread_mutex_unlock(&room->room_mutex);
}
```

#### 9.3.2. Message Ordering Race ✅ FIXED
**Vấn đề ban đầu:**
- MSG_WORD_TO_DRAW có thể arrive sau MSG_ROUND_START
- Drawer không thấy từ cần vẽ

**Giải pháp:**
- Thêm word vào game start/round start data
- Client check 3 nơi để nhận word:
  1. `handleGameStart(data)` - if `data.word` exists
  2. `handleRoundStart(data)` - if `data.word` exists
  3. `handleWordToDraw(data)` - dedicated message
- Fallback mechanism đảm bảo drawer luôn nhận được word

### 9.4. Canvas Synchronization Issues

#### 9.4.1. Other Players Cannot See Drawing ✅ FIXED
**Vấn đề ban đầu:**
- Drawer vẽ nhưng người khác không thấy
- Root cause: Stroke data được wrap 2 lần

**Chi tiết:**
```javascript
// Client gửi:
{
    "type": 100,
    "data": {
        "x1": 100, "y1": 150,
        "color": 16711680  // Hex color as int
    }
}

// Server wrap lại:
{
    "type": 100,
    "data": {
        "data": {  // ← Nested "data"
            "x1": 100, "y1": 150,
            "color": 16711680
        }
    }
}

// Client parse: stroke.data.data.x1 → undefined
```

**Giải pháp:**
- Fix server broadcast để không wrap data 2 lần
- Sử dụng color palette index (0-9) thay vì hex color
- Test kỹ message format

#### 9.4.2. Color Not Synchronized ✅ FIXED
**Vấn đề:**
- Drawer dùng màu khác black, người khác không thấy màu đó
- Root cause: Hex color → int conversion sai

**Giải pháp:**
- Implement 10-color palette với fixed indices:
  ```javascript
  colors = [
      '#000000', // 0: Black
      '#FF0000', // 1: Red
      '#00FF00', // 2: Green
      // ... 7 more colors
  ]
  ```
- Gửi color index thay vì color value
- Đơn giản hóa protocol và đảm bảo consistency

#### 9.4.3. Canvas Blank After Round 2 ✅ FIXED
**Vấn đề ban đầu:**
- Round 1 OK, từ round 2 trở đi canvas trắng
- Root cause: Clear canvas không được broadcast đúng

**Giải pháp:**
- Clear canvas khi `handleRoundStart()`
- Clear strokes array trên server: `room->stroke_count = 0`
- Broadcast clear command đến tất cả players

### 9.5. Network Issues

#### 9.5.1. Remote Server Connection ✅ FIXED
**Vấn đề:**
- Pygame client kết nối đến server remote (192.168.1.2)
- Bị firewall block connections
- Connection timeout

**Root cause:**
- Firewall trên server machine block incoming TCP connections
- Port 9090 không được mở

**Giải pháp:**
```bash
# Open firewall port on server
sudo ufw allow 9090/tcp

# Or disable firewall for testing
sudo ufw disable
```

**Testing:**
```bash
# From client machine
telnet 192.168.1.2 9090

# If successful, connection works
```

**Result:** ✅ Client có thể connect từ LAN machines khác

#### 9.5.2. JSON Parsing Issues ✅ FIXED
**Vấn đề:**
- Server C code parse JSON không handle whitespace sau colon
- Python `json.dumps()` tạo `"key": value` (có space)
- Server expect `"key":value` (không space)
- Parse failed

**Example:**
```python
# Python generates:
{"type": 2, "data": {"username": "Alice"}}
       ↑        ↑                  ↑  spaces!

# Server expected:
{"type":2,"data":{"username":"Alice"}}
```

**Giải pháp:**
Update `server/utils/json.c` để skip whitespace:
```c
const char* json_get_string(const char* json, const char* key, char* out) {
    char* ptr = strstr(json, key);
    if (!ptr) return NULL;
    
    ptr += strlen(key);
    while (*ptr == ':' || *ptr == ' ' || *ptr == '\t') ptr++;  // Skip whitespace
    // ... rest of parsing
}
```

**Result:** ✅ Server parse JSON từ Python client correctly
**Vấn đề:**
- macOS dùng CommonCrypto, Linux dùng OpenSSL
- Byte order functions khác nhau (htobe64)

**Giải pháp:**
- Tạo `server/utils/endian_compat.h`:
  ```c
  #ifdef __APPLE__
      #define htobe64(x) OSSwapHostToBigInt64(x)
  #elif defined(__linux__)
      #include <endian.h>
  #endif
  ```
- Makefile detect OS và link proper libraries
- Crypto macros cho SHA1

### 9.6. Game Logic Issues

#### 9.6.1. Fixed 5-Player Rounds ✅ FIXED
**Vấn đề ban đầu:**
- Game luôn expect 5 players, 5 rounds
- Với 2-3 players, logic không work

**Giải pháp:**
- Dynamic `total_rounds = player_count`
- Track `has_drawn` cho mỗi player
- Skip players đã vẽ hoặc disconnect
- Recalculate rounds khi player leaves

```c
void start_game(Room* room) {
    room->total_rounds = room->player_count;  // Dynamic
}

void start_next_round(Room* room) {
    // Find next player who hasn't drawn
    do {
        room->current_drawer_idx = 
            (room->current_drawer_idx + 1) % room->player_count;
    } while (room->players[room->current_drawer_idx]->has_drawn);
    
    room->players[room->current_drawer_idx]->has_drawn = true;
}

int remove_player_from_room(Room* room, Player* player) {
    // Recalculate total rounds
    int remaining = 0;
    for (int i = 0; i < room->player_count; i++) {
        if (!room->players[i]->has_drawn) remaining++;
    }
    room->total_rounds = room->round_number + remaining;
}
```

#### 9.6.2. Guesser Not Seeing Correct Notification ✅ FIXED
**Vấn đề:**
- Player đoán đúng nhưng không thấy green notification
- Other players thấy nhưng guesser không thấy

**Root cause:**
- Race condition giữa MSG_GUESS_CORRECT và MSG_CHAT_BROADCAST
- Client có thể miss message

**Giải pháp:**
- Thêm logging để debug
- Thêm special status message cho guesser:
  ```javascript
  if (data.player_id === this.playerId) {
      this.showStatus('Correct! 🎉', 'success');
  }
  ```
- Đảm bảo notification luôn hiển thị

#### 9.6.3. Round Count UI Not Updating ✅ FIXED
**Vấn đề:**
- UI hardcoded "Round: X/5"
- Không reflect dynamic total_rounds

**Giải pháp:**
- Server gửi `total_rounds` trong JSON
- HTML: `<span id="total-rounds">0</span>`
- JavaScript update cả hai:
  ```javascript
  document.getElementById('round-number').textContent = data.round;
  document.getElementById('total-rounds').textContent = data.total_rounds;
  ```

### 9.7. Memory Management Issues

#### 9.7.1. Memory Leaks ⚠️
**Potential issues chưa fully test:**
- JSON strings allocated với `malloc()` có thể leak
- Player disconnect không cleanup hết state
- Stroke array có thể overflow với long games

**Best practices cần áp dụng:**
```c
// Always free allocated JSON
char* json = json_create_room_state(room);
broadcast_to_room(room, MSG_TYPE, json, NULL);
free(json);  // ← Important!

// Cleanup on player disconnect
void cleanup_player(Player* player) {
    if (player->recv_buffer) {
        // Clear buffer
        memset(player->recv_buffer, 0, BUFFER_SIZE);
    }
    // Reset all fields
    memset(player, 0, sizeof(Player));
}

// Limit stroke array
void add_stroke(Room* room, const Stroke* stroke) {
    if (room->stroke_count >= MAX_STROKES) {
        // Either reject or overwrite oldest
        return;
    }
    room->strokes[room->stroke_count++] = *stroke;
}
```

#### 9.7.2. Buffer Overflows ⚠️
**Potential issues:**
- Username không check length trước copy
- Chat messages có thể vượt MAX_CHAT_LEN
- Room code có thể malformed

**Safe practices:**
```c
// Safe string copy
void set_username(Player* player, const char* username) {
    strncpy(player->username, username, MAX_USERNAME - 1);
    player->username[MAX_USERNAME - 1] = '\0';  // Ensure null-term
}

// Validate input
bool validate_room_code(const char* code) {
    if (strlen(code) != 6) return false;
    for (int i = 0; i < 6; i++) {
        if (!isalnum(code[i])) return false;
    }
    return true;
}
```

### 9.8. Performance Issues

#### 9.8.1. Broadcasting Overhead ⚠️
**Issue:**
- Mỗi stroke broadcast riêng lẻ
- Với fast drawing, có thể gửi 100+ strokes/second

**Potential optimization:**
```c
// Batch strokes
typedef struct {
    Stroke strokes[BATCH_SIZE];
    int count;
    uint64_t last_send;
} StrokeBatch;

void add_stroke_to_batch(StrokeBatch* batch, const Stroke* stroke) {
    batch->strokes[batch->count++] = *stroke;
    
    uint64_t now = get_current_time_ms();
    if (batch->count >= BATCH_SIZE || 
        now - batch->last_send > 16) {  // 60fps
        flush_stroke_batch(batch);
    }
}
```

#### 9.8.2. JSON Parsing ⚠️
**Issue:**
- Custom JSON parsing với string operations
- Không efficient cho large messages

**Better approach:**
- Sử dụng library như cJSON hoặc jansson
- Binary protocol cho performance-critical messages (strokes)

---

## 10. KẾT LUẬN

### 10.1. Thành tựu đạt được

Dự án Scribble đã hoàn thành các mục tiêu chính:

✅ **Architecture Design**
- Multi-threaded C server với HTTP, TCP, UDP
- 4-thread client proxy bridge
- WebSocket integration cho browser clients
- Clean separation of concerns

✅ **Core Gameplay**
- Turn-based drawing và guessing
- Real-time canvas synchronization
- Dynamic rounds (2-5 players)
- Scoring system với time-based points
- Auto matchmaking và private rooms

✅ **Network Programming**
- Per-client TCP connections
- Message broadcasting với proper routing
- Cross-platform compatibility (macOS, Linux, WSL)
- WebSocket protocol implementation
- JSON message format

✅ **User Experience**
- Modern responsive web UI
- 10-color palette với visual feedback
- Real-time timer và countdown
- Game end rankings với crown icon
- Smooth drawing với touch support

### 10.2. Kỹ năng học được

**Network Programming:**
- Berkeley sockets (TCP, UDP)
- Multi-threading với pthreads
- Thread synchronization (mutex, condition variables)
- WebSocket protocol
- Message serialization/deserialization

**System Design:**
- Client-server architecture
- Message queue design
- State management
- Thread-safe data structures

**C Programming:**
- Memory management
- Pointer manipulation
- Cross-platform development (.dylib/.so)
- Build systems (Makefile)
- Shared library compilation
- ctypes integration

**Python Programming:**
- Pygame framework
- Event-driven programming
- ctypes FFI (Foreign Function Interface)
- JSON serialization
- Non-blocking I/O

**Game Development:**
- Game loop design (60 FPS)
- State machine implementation
- Real-time rendering
- Input handling
- UI component design

### 10.3. Hạn chế và cải tiến

**Hạn chế hiện tại:**
1. Chưa có latency-based matchmaking
2. Reconnection system chưa hoàn chỉnh (canvas restore)
3. Thiếu mutex protection ở một số race conditions
4. Memory leaks potential chưa fully test
5. Performance chưa optimize cho scale lớn
6. Pygame client single-threaded (blocking receive)

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
   - Spectator mode
   - Undo drawing
   - Hint system
   - Advanced scoring (drawer points)
   - Player profiles
   - Leaderboards
   - Custom word lists
   - Drawing time limit options

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

### 10.4. Tổng kết

Scribble project đã successfully implement một multiplayer game hoàn chỉnh với:
- **2000+ lines** of C code (server + networking library)
- **800+ lines** of Python code (Pygame client)
- **2 concurrent threads** (TCP server + Timer)
- **1 network protocol** (TCP với JSON, simplified từ TCP/UDP/WebSocket)
- **30+ message types** cho game communication
- **10+ features** hoàn chỉnh

**Major Architectural Decisions:**
1. ✅ **Migration từ Web UI sang Pygame:** Simplified deployment, better performance
2. ✅ **Complete UDP reversion:** TCP-only approach, KISS principle
3. ✅ **C networking library:** Reusable, cross-platform, efficient
4. ✅ **ctypes integration:** Clean Python-C bridge
5. ✅ **JSON protocol:** Simple, debuggable, maintainable

**Technical Achievements:**
- Low-level socket programming (Berkeley sockets)
- Multi-threaded server với select() multiplexing
- Cross-platform shared library compilation
- Foreign Function Interface (ctypes)
- Real-time game synchronization
- State machine implementation
- Event-driven architecture

**Lessons Learned:**
1. **Simplicity wins:** TCP-only đơn giản hơn và đủ tốt
2. **Premature optimization is evil:** UDP không cần thiết cho use case này
3. **Debug-ability matters:** JSON > Binary cho development
4. **Architecture evolution:** OK để pivot khi design không work
5. **Testing is crucial:** Real-world testing exposed many issues

Project demonstrate understanding của:
- Low-level network programming
- Concurrent programming
- System architecture design
- Real-time synchronization
- Cross-platform development
- Game development fundamentals

Đây là foundation tốt để phát triển thành production-ready game server với proper testing, security, và scalability. Codebase hiện tại clean, maintainable, và ready cho future enhancements.

---

## PHỤ LỤC

### A. Cấu trúc File quan trọng

**server/protocol.h** - Core data structures và message types
**server/game/game_logic.c** - Game mechanics
**server/tcp/tcp_handler.c** - Message handlers
**server/tcp/tcp_server.c** - TCP server với select()
**client_c/network.c** - C networking library
**client_pygame/main.py** - Pygame game client
**client_pygame/network_wrapper.py** - ctypes wrapper
**client_pygame/protocol.py** - Message type definitions

### B. Message Flow Diagrams

Xem Section 6.3 cho chi tiết Complete Game Flow

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

### D. Testing Checklist

- [ ] 2 players can join và play
- [ ] 5 players full room
- [ ] Drawing syncs correctly via TCP
- [ ] All 10 colors work
- [ ] Brush size changes work
- [ ] Guessing awards points correctly
- [ ] Timer counts down properly
- [ ] Game ends with rankings
- [ ] Private room works with code
- [ ] Player disconnect handled gracefully
- [ ] Reconnection restores state (partial)
- [ ] Remote connection works (LAN)
- [ ] Clear canvas works
- [ ] Chat messages appear
- [ ] Drawing tools show/hide per turn

### E. Deprecated Components

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
