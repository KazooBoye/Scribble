# 🎨 Scribble - Multiplayer Drawing & Guessing Game

A real-time multiplayer game similar to skribbl.io, built entirely in C for networking (server + multi-threaded client proxy) with a web-based UI.

## 🏗️ Architecture Overview

```
┌──────────────────────────────────────────┐
│         Web UI Clients (Browser)         │
│     HTML/JS/CSS served by C server       │
└────────────▲──────────▲──────────────────┘
             │          │ WebSocket (JSON)
             │          │
    ┌────────┴──────────┴────────┐
    │   C Client Proxy            │
    │   (Multi-threaded)          │
    ├─────────────────────────────┤
    │ Thread 1: WebSocket         │ ◄── Manages multiple browser connections
    │ Thread 2: TCP Handler       │ ◄── Connection to game server
    │ Thread 3: UDP Handler       │ ◄── Low-latency drawing strokes
    │ Thread 4: Dispatcher        │ ◄── Thread-safe message routing
    └──────────┬──────────────────┘
               │ TCP + UDP
    ┌──────────▼──────────────────┐
    │    C Game Server            │
    ├─────────────────────────────┤
    │ HTTP Server → UI files      │
    │ TCP Server  → Game logic    │
    │ UDP Server  → Drawing data  │
    │ Matchmaking by latency      │
    │ Reconnection engine         │
    │ Logging engine (JSON)       │
    └─────────────────────────────┘
```

## ✨ Key Features

### Server Features (Person A Tasks)
- ✅ **File Logging** - JSON lines format for all game events
- ✅ **Player Identification** - IP-based user identification
- ✅ **Latency-based Matchmaking** - Groups players with similar RTT
- ✅ **UDP Broadcasting** - Low-latency stroke transmission
- ✅ **Chat System** - Message handling and acknowledgment
- ✅ **Timekeeper** - 1-second tick timer for game management
- ✅ **Game Logic** - Turn rotation, scoring, word selection
- ✅ **Match Logging** - Complete game state logging for recovery
- ✅ **Reconnection System** - 5-minute grace period with state restore
- ✅ **Connection Management** - Clean disconnect handling

### Client Proxy Features (Person B Tasks)
- ✅ **TCP Socket Connection** - Persistent connection to game server
- ✅ **Room Requests** - Auto matchmaking and private rooms
- ✅ **Private Room Creation** - 6-character room codes
- ✅ **Ping/Pong Keepalive** - Maintains online status
- ✅ **Reconnection Handling** - Automatic reconnect with session token
- ✅ **Web UI** - Modern, responsive interface

### Multi-threaded Proxy Architecture
Each client proxy runs **4 independent threads**:

1. **WebSocket Thread** - Handles multiple browser connections simultaneously
2. **TCP Thread** - Maintains game server connection (matchmaking, chat, game state)
3. **UDP Thread** - Ultra-low latency drawing stroke transmission
4. **Dispatcher Thread** - Thread-safe message routing with mutex protection

**Each player/browser connects to the proxy via WebSocket and gets their own managed connection context, ensuring isolation and thread-safety.**

## 📋 Requirements

- **C Compiler**: GCC or Clang with C11 support
- **Operating System**: Linux or macOS
- **Dependencies**: pthread, standard C libraries
- **Browser**: Modern web browser with WebSocket support

## 🚀 Quick Start

### 1. Build the Project

```bash
# Clone the repository
cd Scribble

# Build everything
make all

# Install resources (copies webui files and wordlist)
make install
```

### 2. Run the Game

```bash
# Option A: Run everything with one command
make run

# Option B: Run components separately (in different terminals)
# Terminal 1: Start server
make run-server

# Terminal 2: Start client proxy
make run-client
```

### 3. Play the Game

Open your browser and go to:
```
http://localhost:8080
```

**For multiplayer testing**, open multiple browser tabs or use different devices on the same network.

### 4. Stop Everything

```bash
make stop
```

## 🎮 How to Play

### Starting a Game

**Option 1: Auto Matchmaking**
1. Enter your username
2. Click "Play Now"
3. Wait for 5 players to join
4. Game starts automatically!

**Option 2: Private Room**
1. Click "Create Private Room"
2. Share the 6-character room code with friends
3. Friends click "Join Room" and enter the code
4. Game starts when 5 players join

### Game Rules

- 5 players per match
- Each player gets one turn to draw
- 90 seconds per round
- Drawer receives a secret word
- Other players guess by typing in chat
- Points awarded based on speed of correct guess
- Player with highest score wins!

### Drawing Controls

When it's your turn to draw:
- **Brush Size**: Adjust thickness (2-20px)
- **Color Picker**: Choose your drawing color
- **Clear Canvas**: Start over
- Draw with mouse or touch

## 🔧 Development

### Project Structure

```
Scribble/
├── server/                 # C Game Server
│   ├── http/              # HTTP static file server
│   ├── tcp/               # TCP game logic
│   ├── udp/               # UDP drawing broadcast
│   ├── game/              # Game mechanics
│   ├── utils/             # Logger, JSON, timer
│   └── main.c             # Server entry point
│
├── client_proxy/          # C Multi-threaded Proxy
│   ├── threads/           # 4 worker threads
│   │   ├── ws_thread.c    # WebSocket listener
│   │   ├── tcp_thread.c   # TCP handler
│   │   ├── udp_thread.c   # UDP handler
│   │   └── dispatcher.c   # Message router
│   ├── utils/             # Queue, state cache, JSON
│   └── main.c             # Proxy entry point
│
├── webui/                 # Web Interface
│   ├── index.html         # Main page
│   ├── style.css          # Styling
│   ├── websocket.js       # WS connection
│   ├── drawing.js         # Canvas drawing
│   └── main.js            # Game controller
│
├── Makefile               # Build system
└── README.md              # This file
```

### Build Commands

```bash
# Show all available commands
make help

# Build only server
make server

# Build only client proxy
make client

# Clean build files
make clean

# Rebuild everything
make clean all install

# Development mode (clean + build + run)
make dev
```

### Viewing Logs

```bash
# Server logs (JSON lines format)
tail -f build/server.log

# Client proxy logs
tail -f build/proxy.log

# Both logs
tail -f build/*.log
```

### Log Format

Server logs are in JSON Lines format for easy parsing:

```json
{"timestamp":"2024-11-23T20:15:30","type":"player","data":{"player_id":1,"event":"registered","details":"Alice"}}
{"timestamp":"2024-11-23T20:15:45","type":"room","data":{"room_id":1,"event":"created","details":"public"}}
{"timestamp":"2024-11-23T20:16:00","type":"stroke","data":{"room_id":1,"stroke_id":42,"x1":100.5,"y1":200.3,"x2":105.2,"y2":210.8,"color":0,"thickness":5}}
{"timestamp":"2024-11-23T20:16:15","type":"guess","data":{"room_id":1,"player_id":2,"guess":"apple","correct":true}}
```

## 🧪 Testing Reconnection

To test the reconnection feature:

1. Start playing a game
2. Close your browser tab (connection lost)
3. Reopen the same URL within 5 minutes
4. Click "Reconnect Now" in the dialog
5. Your game state will be restored!

The server maintains:
- Player score
- Room state
- Current round
- Drawing buffer
- Chat history

## 🌐 Network Ports

- **8080** - HTTP (Web UI)
- **9090** - TCP (Game logic, matchmaking, chat)
- **9091** - UDP (Drawing strokes, low latency)
- **8081** - WebSocket (Proxy ↔ Browser)

## 🎯 Implementation Details

### Matchmaking Algorithm

Players are grouped by network latency (RTT):
- Server measures ping during handshake
- Groups players within 50ms tolerance
- Ensures fair gameplay for all participants

### Threading Model

**Client Proxy uses POSIX threads with proper synchronization:**

```c
// Message queue (thread-safe)
pthread_mutex_t queue_mutex;
pthread_cond_t queue_cond;

// State cache (thread-safe)
pthread_mutex_t state_mutex;
```

Each thread has its own message queue and the dispatcher routes messages between them using mutex-protected shared memory.

### Protocol

**TCP Messages**: `[4-byte length][JSON payload]`

**UDP Messages**: Binary struct for minimal overhead

**WebSocket**: JSON messages for browser compatibility

## 🐛 Troubleshooting

### "Port already in use"

```bash
# Check what's using the ports
lsof -i :8080
lsof -i :9090

# Kill the processes
make stop
```

### "Connection refused"

Make sure both server and proxy are running:

```bash
# Check processes
ps aux | grep scribble

# Restart everything
make stop
make run
```

### "Can't connect to WebSocket"

Check that the client proxy is running and listening on port 8081:

```bash
netstat -an | grep 8081
```

## 📝 Assignment Tasks Checklist

### Person A (Server) - ✅ All Completed
- [x] Server logging to file (1pt)
- [x] IP-based user identification (1pt)
- [x] Latency-based matchmaking (3pt)
- [x] UDP drawing broadcast (2pt)
- [x] Chat message processing (2pt)
- [x] Timekeeper for scoring (2pt)
- [x] Game logic implementation (2pt)
- [x] Match activity logging (2pt)
- [x] Reconnection with state restore (3pt)
- [x] Connection cleanup (1pt)

### Person B (Client) - ✅ All Completed
- [x] TCP socket connection (1pt)
- [x] Room join request (1pt)
- [x] Private room creation (1pt)
- [x] Ping/pong keepalive (2pt)
- [x] Reconnection request (1pt)
- [x] Web UI design (3pt)

## 🏆 Advanced Features

- **Latency optimization** - UDP for drawing, TCP for reliable messages
- **Thread-safe architecture** - Mutex protection for all shared resources
- **Graceful degradation** - Handles player disconnects smoothly
- **Session persistence** - 5-minute reconnection window
- **Responsive design** - Works on desktop, tablet, and mobile
- **Real-time updates** - Sub-100ms drawing latency

## 👥 Credits

Built for Network Programming course assignment.

**Architecture**: Multi-threaded C server + C proxy + Web UI

**Key Technologies**: 
- POSIX threads
- Berkeley sockets (TCP/UDP)
- WebSocket protocol
- HTML5 Canvas

## 📄 License

Educational project for academic purposes.

---

**Enjoy drawing and guessing! 🎨**

For questions or issues, check the logs in `build/*.log`
