# SCRIBBLE - PRESENTATION SLIDES
## Multiplayer Drawing & Guessing Game

**Hướng dẫn tạo slides từ file này**

---

## 🎯 SLIDE DESIGN GUIDELINES

### Tool Recommendations:
1. **Google Slides** - Free, collaborative
2. **PowerPoint** - Professional features
3. **Keynote** (Mac) - Beautiful transitions
4. **Canva** - Modern templates
5. **reveal.js** - Web-based (for developers)

### Design Tips:
- **Màu chủ đạo:** #667eea (Purple/Blue gradient)
- **Font:** Roboto, Inter, hoặc San Francisco
- **Layout:** Minimalist với nhiều whitespace
- **Icons:** Use emojis hoặc Font Awesome
- **Code snippets:** Solarized Dark theme

---

## SLIDE 1: TITLE SLIDE

```
═══════════════════════════════════════════════════════════════

                        🎨 SCRIBBLE
           Multiplayer Drawing & Guessing Game
              Built with C & WebSocket Technology

═══════════════════════════════════════════════════════════════

                      Cao Duc Anh
                Network Programming Course
                   December 7, 2025

```

**Background:** Purple-blue gradient  
**Animation:** Fade in title, then subtitle  

---

## SLIDE 2: AGENDA

```
📋 NỘI DUNG TRÌNH BÀY

1. 🎮  Project Overview
2. 🏗️  System Architecture  
3. 💻  Technology Stack
4. ✨  Key Features
5. 🔄  Message Flow
6. 🐛  Challenges & Solutions
7. 📊  Achievements
8. 🚀  Future Work
9. 🎬  Live Demo
```

**Layout:** 2 columns, icons bên trái  
**Animation:** Bullet points appear sequentially  

---

## SLIDE 3: WHAT IS SCRIBBLE?

```
═══════════════════════════════════════════════════════════════

              🎮 SCRIBBLE - REAL-TIME GAME

        ┌─────────────────────────────────────┐
        │  One player DRAWS 🎨                │
        │         ↓                            │
        │  Others GUESS the word 💭           │
        │         ↓                            │
        │  Fastest guess = More points 🏆     │
        └─────────────────────────────────────┘

═══════════════════════════════════════════════════════════════
```

**Comparison:**
```
Similar to:  skribbl.io, Gartic Phone, Drawful
Built with:  100% C backend + Modern Web UI
Players:     2-5 per match
Rounds:      Dynamic (equals player count)
```

**Visual:** Screenshot hoặc GIF của gameplay  

---

## SLIDE 4: PROJECT GOALS

```
🎯 MỤC TIÊU DỰ ÁN

✅  Build scalable game server in C
✅  Implement real-time networking
✅  Multi-threaded architecture
✅  WebSocket bridge for browsers
✅  Cross-platform compatibility
✅  Modern responsive UI
```

**Animation:** Checkmarks appear với bounce effect  
**Background:** Subtle pattern  

---

## SLIDE 5: ARCHITECTURE OVERVIEW

```
┌────────────────────────────────────────────────────────┐
│                   WEB BROWSERS                          │
│            HTML5 Canvas + WebSocket                     │
└─────────────────────┬──────────────────────────────────┘
                      │ WS (8081)
                      ↓
┌────────────────────────────────────────────────────────┐
│                CLIENT PROXY (C)                         │
│                4 Concurrent Threads                     │
├────────────┬────────────┬────────────┬─────────────────┤
│ WebSocket  │    TCP     │    UDP     │   Dispatcher    │
│  Thread    │   Thread   │   Thread   │     Thread      │
└────────────┴────────────┴────────────┴─────────────────┘
                      │ TCP (9090) + UDP (9091)
                      ↓
┌────────────────────────────────────────────────────────┐
│                  GAME SERVER (C)                        │
├─────────────┬──────────────┬─────────────┬────────────┤
│    HTTP     │     TCP      │     UDP     │    Game    │
│  (8080)     │   (9090)     │   (9091)    │   Engine   │
└─────────────┴──────────────┴─────────────┴────────────┘
```

**Animation:** Components appear từ top → bottom  
**Highlight:** Each layer khi explain  

---

## SLIDE 6: TECHNOLOGY STACK

```
╔════════════════════════════════════════════════════════╗
║                    BACKEND (C11)                        ║
╠════════════════════════════════════════════════════════╣
║  🔹 Berkeley Sockets (TCP/UDP)                         ║
║  🔹 POSIX Threads (pthread)                            ║
║  🔹 WebSocket Protocol                                 ║
║  🔹 JSON Message Format                                ║
║  🔹 Cross-platform (macOS/Linux/WSL)                   ║
╚════════════════════════════════════════════════════════╝

╔════════════════════════════════════════════════════════╗
║                  FRONTEND (Web)                         ║
╠════════════════════════════════════════════════════════╣
║  🔹 HTML5 Canvas API                                   ║
║  🔹 ES6+ JavaScript                                    ║
║  🔹 WebSocket Client                                   ║
║  🔹 Responsive CSS (Flexbox)                           ║
╚════════════════════════════════════════════════════════╝
```

**Layout:** 2 boxes với gradient borders  
**Icons:** Tech logos nếu có  

---

## SLIDE 7: CLIENT PROXY - 4 THREADS

```
═══════════════════════════════════════════════════════════

         🧵 MULTI-THREADED CLIENT PROXY

┌─────────────────────────────────────────────────────────┐
│                                                           │
│   Thread 1: WebSocket Listener                          │
│   ├─ Accept browser connections                         │
│   ├─ WebSocket handshake (SHA1)                        │
│   └─ Parse frames → Enqueue                            │
│                                                           │
│   Thread 2: TCP Handler                                 │
│   ├─ Persistent connection to server                    │
│   ├─ Send/receive game messages                         │
│   └─ 4-byte length prefix protocol                      │
│                                                           │
│   Thread 3: UDP Handler                                 │
│   └─ Low-latency stroke transmission                    │
│                                                           │
│   Thread 4: Dispatcher (Router)                         │
│   ├─ Thread-safe message queue                          │
│   ├─ Mutex + Condition variables                        │
│   └─ Route messages between threads                     │
│                                                           │
└─────────────────────────────────────────────────────────┘
```

**Animation:** Each thread highlights sequentially  
**Visual:** Thread diagram với arrows  

---

## SLIDE 8: KEY FEATURES (1/2)

```
✨ CORE GAMEPLAY

🎮  Turn-based Drawing
    └─ Each player draws once per game

⏱️  Dynamic Rounds (2-5)
    └─ Rounds = Number of players

🎨  Real-time Canvas Sync
    └─ Drawing appears instantly for all

💬  Chat & Guessing System
    └─ Case-insensitive matching

⚡  15-Second Countdown
    └─ Game starts when 2+ players join

🏆  Time-based Scoring
    └─ Faster guess = More points
```

**Layout:** Icons lớn bên trái, text bên phải  
**Animation:** Feature items slide in từ left  

---

## SLIDE 9: KEY FEATURES (2/2)

```
✨ ADVANCED FEATURES

🌈  10-Color Palette
    └─ Consistent colors across all players

🎯  Auto Matchmaking
    └─ Join available rooms automatically

🔒  Private Rooms
    └─ 6-character room codes

👥  Player Management
    └─ Handle join/leave dynamically

📊  Game End Rankings
    └─ #1 gets crown 👑

🔌  Reconnection Support
    └─ Resume game after disconnect
```

**Visual:** Screenshots của UI features  
**Animation:** Fade in với scaling  

---

## SLIDE 10: MESSAGE PROTOCOL

```
═══════════════════════════════════════════════════════════

           📡 TCP MESSAGE FORMAT

┌──────────────────────────────────────────────────────────┐
│  [4 bytes: length]  [JSON payload]                       │
└──────────────────────────────────────────────────────────┘

Example:
┌──────────────────────────────────────────────────────────┐
│  0x00 0x00 0x00 0x3A                                     │
│  {"type":10,"data":{"round":1,"players":[...]}}          │
└──────────────────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════

           📦 MESSAGE TYPES (30+)

Game Flow:      REGISTER, JOIN_ROOM, GAME_START
Gameplay:       ROUND_START, YOUR_TURN, WORD_TO_DRAW
Drawing:        STROKE (100), CLEAR_CANVAS (101)
Communication:  CHAT, CHAT_BROADCAST, GUESS_CORRECT
State Updates:  TIMER_UPDATE, SCORE_UPDATE, PLAYER_LEAVE
```

**Code style:** Monospace font với syntax highlighting  
**Visual:** Byte diagram với colors  

---

## SLIDE 11: COMPLETE GAME FLOW

```
┌─────────────────────────────────────────────────────────┐
│                    GAME SEQUENCE                         │
└─────────────────────────────────────────────────────────┘

1️⃣  Connection & Registration
    Browser → MSG_REGISTER → Server
    Server  → MSG_REGISTER_ACK (player_id, token)

2️⃣  Join Matchmaking
    Browser → MSG_JOIN_ROOM (room_id: 0)
    Server  → MSG_ROOM_JOINED (players list)

3️⃣  Countdown (when 2+ players)
    Server  → MSG_COUNTDOWN_UPDATE (every second)
    15... 14... 13... → 1

4️⃣  Game Start
    Server  → MSG_GAME_START (all players)
    Server  → MSG_WORD_TO_DRAW (drawer only)

5️⃣  Drawing Phase
    Drawer  → STROKE messages (continuous)
    Server  → Broadcast to other players

6️⃣  Guessing Phase
    Guesser → MSG_CHAT (with guess)
    Server  → MSG_GUESS_CORRECT (if right)

7️⃣  Round End & Next Round
    Server  → MSG_ROUND_END
    Server  → MSG_ROUND_START (next round)

8️⃣  Game End
    Server  → MSG_GAME_END (rankings)
```

**Animation:** Step-by-step highlight  
**Visual:** Flowchart hoặc sequence diagram  

---

## SLIDE 12: DRAWING SYNCHRONIZATION

```
═══════════════════════════════════════════════════════════

           🎨 REAL-TIME CANVAS SYNC

┌─────────────────────────────────────────────────────────┐
│                                                           │
│  Drawer's Browser                                        │
│       │                                                   │
│       │ Mouse Move Event                                 │
│       ├─ Calculate stroke: {x1, y1, x2, y2}             │
│       ├─ Draw locally (instant feedback)                │
│       ├─ Create JSON message                            │
│       │                                                   │
│       ↓ WebSocket                                        │
│                                                           │
│  Client Proxy                                            │
│       │                                                   │
│       ├─ Enqueue to dispatcher                          │
│       ↓ TCP                                              │
│                                                           │
│  Game Server                                             │
│       │                                                   │
│       ├─ Validate (is drawer?)                          │
│       ├─ Add player_id                                   │
│       ├─ Broadcast to room (except sender)              │
│       │                                                   │
│       ↓ TCP                                              │
│                                                           │
│  Other Players' Browsers                                 │
│       │                                                   │
│       └─ Receive stroke → Draw on canvas                │
│                                                           │
└─────────────────────────────────────────────────────────┘

⚡ Latency: ~10-50ms depending on network
```

**Animation:** Data flow từ top xuống bottom với timing  

---

## SLIDE 13: CHALLENGES & SOLUTIONS (1/3)

```
🐛 CHALLENGE 1: Race Conditions

Problem:
  ⚠️  Timer thread + TCP handler access room state
  ⚠️  No mutex protection
  ⚠️  Potential data corruption

Solution:
  ✅  Add pthread_mutex to Room struct
  ✅  Lock/unlock in all room operations
  ✅  Protect timer_update() and process_guess()

Code:
  pthread_mutex_lock(&room->room_mutex);
  room->time_remaining--;
  if (room->time_remaining <= 0) {
      end_round(room);
  }
  pthread_mutex_unlock(&room->room_mutex);
```

**Layout:** Problem → Solution → Code  
**Colors:** Red cho problem, green cho solution  

---

## SLIDE 14: CHALLENGES & SOLUTIONS (2/3)

```
🐛 CHALLENGE 2: Canvas Not Syncing

Problem:
  ⚠️  Drawer vẽ nhưng others không thấy
  ⚠️  Root cause: Stroke data wrapped twice
  
  {
    "type": 100,
    "data": {
      "data": {  ← Nested!
        "x1": 100
      }
    }
  }

Solution:
  ✅  Fix server broadcast format
  ✅  Use color palette index (0-9)
  ✅  Test message structure carefully

Result:
  ✨  Drawing syncs perfectly
  🎨  All 10 colors work correctly
```

**Visual:** Before/After JSON comparison  
**Highlight:** The nested "data" problem  

---

## SLIDE 15: CHALLENGES & SOLUTIONS (3/3)

```
🐛 CHALLENGE 3: Fixed 5-Player Logic

Problem:
  ⚠️  Game expects exactly 5 players
  ⚠️  2-3 players couldn't play properly

Solution:
  ✅  Dynamic total_rounds = player_count
  ✅  Track has_drawn for each player
  ✅  Skip disconnected players
  ✅  Recalculate on player leave

Code:
  void start_game(Room* room) {
      room->total_rounds = room->player_count;
      // 2 players = 2 rounds
      // 3 players = 3 rounds
  }

Result:
  ✨  Game works with 2-5 players
  ✨  Rounds adjust dynamically
```

**Visual:** Diagram showing flexible player counts  

---

## SLIDE 16: CROSS-PLATFORM SUPPORT

```
🌍 CROSS-PLATFORM COMPATIBILITY

┌─────────────────────────────────────────────────────────┐
│                                                           │
│  macOS (Darwin)                                          │
│  ├─ CommonCrypto for SHA1                               │
│  ├─ OSSwapHostToBigInt64 for byte order                │
│  └─ ✅ Works perfectly                                   │
│                                                           │
│  Linux                                                   │
│  ├─ OpenSSL for SHA1                                    │
│  ├─ endian.h for htobe64                                │
│  └─ ✅ Works perfectly                                   │
│                                                           │
│  Windows (WSL2)                                          │
│  ├─ Same as Linux                                       │
│  ├─ Port forwarding via PowerShell                      │
│  ├─ OR mirrored networking (.wslconfig)                 │
│  └─ ✅ Works with extra setup                           │
│                                                           │
└─────────────────────────────────────────────────────────┘

Key File: server/utils/endian_compat.h
  → Abstracts platform differences
```

**Icons:** OS logos (Apple, Linux, Windows)  

---

## SLIDE 17: ACHIEVEMENTS - BY THE NUMBERS

```
📊 PROJECT STATISTICS

┌─────────────────────────────────────────────────────────┐
│                                                           │
│  📝  2,000+   Lines of C code                           │
│  📝  1,000+   Lines of JavaScript                       │
│  🧵  4         Concurrent threads (proxy)               │
│  🌐  3         Network protocols (HTTP/TCP/WS)          │
│  📡  30+       Message types                            │
│  ✨  10+       Complete features                        │
│  🎨  10        Color palette options                    │
│  👥  2-5       Players per match                        │
│  ⏱️  90        Seconds per round                        │
│  🎯  15        Second countdown                         │
│                                                           │
└─────────────────────────────────────────────────────────┘
```

**Animation:** Numbers count up  
**Visual:** Large bold numbers  

---

## SLIDE 18: COMPLETED FEATURES ✅

```
✅ FULLY IMPLEMENTED

Core Gameplay
  ✅ Turn-based drawing & guessing
  ✅ Dynamic rounds (2-5 players)
  ✅ Real-time canvas synchronization
  ✅ 10-color palette
  ✅ Scoring system

Multiplayer
  ✅ Auto matchmaking
  ✅ Private rooms with codes
  ✅ 15-second countdown
  ✅ Player join/leave handling
  ✅ No join after round 1

Infrastructure
  ✅ Multi-threaded server
  ✅ 4-thread client proxy
  ✅ WebSocket bridge
  ✅ Cross-platform support
  ✅ JSON logging

UI/UX
  ✅ Responsive web design
  ✅ Game end rankings
  ✅ Chat system
  ✅ Timer display
  ✅ Return to home
```

**Layout:** 4 columns với checkmarks  

---

## SLIDE 19: INCOMPLETE FEATURES ⚠️

```
⚠️ PARTIALLY IMPLEMENTED / TODO

High Priority
  ⚠️  Latency-based matchmaking
      → Basic auto-matching works
      → Need RTT measurement

  ⚠️  Full reconnection system
      → Session tokens work
      → Canvas restore incomplete

Medium Priority
  ❌  Private room passwords
  ❌  Spectator mode
  ❌  Undo drawing stroke
  ❌  Advanced scoring (drawer points)
  ❌  Hint system

Low Priority
  ❌  Player profiles
  ❌  Leaderboards
  ❌  Chat history
  ❌  Drawing replay
```

**Colors:** Yellow cho partial, red cho missing  

---

## SLIDE 20: FUTURE IMPROVEMENTS

```
🚀 ROADMAP FOR ENHANCEMENT

Scalability
  🔹 Multiple server instances
  🔹 Load balancing
  🔹 Redis for shared state
  🔹 Database persistence

Performance
  🔹 Stroke batching (60fps)
  🔹 Binary protocol option
  🔹 Connection pooling
  🔹 Memory optimization

Security
  🔹 User authentication
  🔹 Rate limiting
  🔹 Input validation
  🔹 XSS protection

Features
  🔹 Custom word lists
  🔹 Multiple languages
  🔹 Power-ups
  🔹 Tournament mode
```

**Visual:** Roadmap timeline hoặc mind map  

---

## SLIDE 21: TECHNICAL LEARNINGS

```
💡 SKILLS GAINED

Network Programming
  ✓ Berkeley sockets (TCP/UDP)
  ✓ Multi-threading với pthreads
  ✓ Thread synchronization
  ✓ WebSocket protocol
  ✓ Message queues

System Design
  ✓ Client-server architecture
  ✓ Multi-threaded proxy pattern
  ✓ State management
  ✓ Race condition handling

C Programming
  ✓ Memory management
  ✓ Pointer manipulation
  ✓ Cross-platform coding
  ✓ Build systems (Makefile)

Web Development
  ✓ Canvas API
  ✓ WebSocket client
  ✓ Event-driven JS
  ✓ Responsive design
```

**Icons:** 💻 🔧 🎓  

---

## SLIDE 22: ARCHITECTURE STRENGTHS

```
✨ DESIGN HIGHLIGHTS

1. Separation of Concerns
   ├─ Server: Pure C game logic
   ├─ Proxy: Bridge layer
   └─ Client: Modern web UI

2. Thread Safety
   ├─ Message queues with mutex
   ├─ Condition variables
   └─ No shared state between threads

3. Scalability Patterns
   ├─ Per-client connections
   ├─ Room-based isolation
   └─ Stateless message handling

4. Performance Optimizations
   ├─ Low-latency drawing (UDP-ready)
   ├─ Efficient broadcasting
   └─ Minimal client-side processing

5. Maintainability
   ├─ Clear module structure
   ├─ Consistent naming
   └─ Comprehensive logging
```

---

## SLIDE 23: CODE QUALITY METRICS

```
📈 CODE QUALITY

✅ Strengths
  • Modular design
  • Clear separation of layers
  • Consistent error handling
  • Comprehensive logging
  • Cross-platform compatibility

⚠️ Areas for Improvement
  • Add unit tests
  • More race condition protection
  • Memory leak testing
  • Performance profiling
  • Code documentation

🔧 Testing Performed
  • Manual multiplayer testing
  • Cross-platform compilation
  • Network disconnect scenarios
  • Drawing synchronization
  • Round progression logic
```

---

## SLIDE 24: DEMO PREPARATION

```
🎬 LIVE DEMO CHECKLIST

Before Demo:
  ☑️  Start game server
  ☑️  Start client proxy
  ☑️  Open 2+ browser tabs
  ☑️  Prepare multiple devices (optional)

Demo Flow:
  1️⃣  Show landing page
  2️⃣  Register 2 players
  3️⃣  Join auto matchmaking
  4️⃣  Wait for countdown
  5️⃣  Draw something (color palette)
  6️⃣  Guess from other player
  7️⃣  Show score update
  8️⃣  Complete round
  9️⃣  Show game end rankings
  🔟  Return to home

Backup Plan:
  • Screenshots/video recording
  • Terminal logs showing messages
```

---

## SLIDE 25: Q&A PREPARATION

```
❓ ANTICIPATED QUESTIONS

Technical Questions:
  Q: Why not use WebRTC for peer-to-peer?
  A: Server-authoritative prevents cheating

  Q: Why C instead of Node.js/Python?
  A: Learning low-level networking + performance

  Q: How handle disconnections?
  A: Session tokens + state preservation (5 min)

  Q: Scale to 1000s of players?
  A: Need multiple servers + load balancer

Design Questions:
  Q: Why 4 threads in proxy?
  A: Separate concerns + thread safety

  Q: Why WebSocket instead of direct TCP?
  A: Browser compatibility

  Q: Security concerns?
  A: No auth yet - planned for future
```

---

## SLIDE 26: DEMO VIDEO

```
═══════════════════════════════════════════════════════════

           🎬 LIVE DEMONSTRATION

         [Insert Demo Video or GIF Here]

          Showing:
          • Player registration
          • Auto matchmaking
          • Real-time drawing
          • Guessing mechanic
          • Score updates
          • Game end rankings

═══════════════════════════════════════════════════════════

          Alternative: Live Demo on Stage
```

**Visual:** Embedded video hoặc animated GIF  
**Backup:** Screenshots sequence  

---

## SLIDE 27: REPOSITORY & DOCUMENTATION

```
📦 PROJECT RESOURCES

GitHub Repository
  🔗 github.com/KazooBoye/Scribble

Documentation
  📄 README.md - Setup & usage
  📄 TECHNICAL_REPORT.md - Full documentation
  📄 ARCHITECTURE_DIAGRAM.txt - System design
  📄 PROJECT_SUMMARY.md - Quick overview

Build & Run
  $ make clean && make all && make install
  $ make run
  $ open http://localhost:8080

Files
  📂 server/        - C game server
  📂 client_proxy/  - Multi-threaded proxy
  📂 webui/         - Web interface
  📂 report_docs/   - Project reports
```

**QR Code:** Link đến GitHub repository  

---

## SLIDE 28: ACKNOWLEDGMENTS

```
🙏 ACKNOWLEDGMENTS

Course
  • Network Programming
  • Instructor: [Instructor Name]
  • Semester: Fall 2025

Resources
  • Beej's Guide to Network Programming
  • WebSocket Protocol RFC 6455
  • POSIX Threads Programming
  • Canvas API Documentation

Testing
  • Friends & classmates
  • Multiple browsers & devices

Inspiration
  • skribbl.io
  • Gartic Phone
  • Drawful
```

---

## SLIDE 29: CONCLUSION

```
═══════════════════════════════════════════════════════════

                    🎯 CONCLUSION

✅  Successfully built real-time multiplayer game

✅  Demonstrated low-level network programming

✅  Multi-threaded architecture with thread safety

✅  Cross-platform compatibility

✅  Modern web UI with WebSocket

═══════════════════════════════════════════════════════════

         Key Takeaway: C + WebSockets = ❤️

        Low-level control meets modern web

═══════════════════════════════════════════════════════════
```

**Animation:** Triumphant fade in  
**Background:** Gradient celebration theme  

---

## SLIDE 30: THANK YOU

```
═══════════════════════════════════════════════════════════

                    THANK YOU!

                  Questions? 🤔

═══════════════════════════════════════════════════════════

                   Cao Duc Anh
           caoducanh@example.com (optional)
         🔗 github.com/KazooBoye/Scribble

═══════════════════════════════════════════════════════════

            🎮 Want to play? Let's demo! 🎨

═══════════════════════════════════════════════════════════
```

**Background:** Fun, colorful design  
**Animation:** Confetti effect (optional)  

---

## 🎨 BONUS: TRANSITION STYLES

**Suggested Transitions:**
- **Slide 1-5:** Fade
- **Slide 6-10:** Slide from right
- **Slide 11-15:** Zoom in
- **Slide 16-20:** Push
- **Slide 21-25:** Fade
- **Slide 26-30:** Dissolve

**Timing:** 0.5-0.8 seconds per transition

---

## 📊 BONUS: APPENDIX SLIDES

### APPENDIX A: Code Structure

```
Scribble/
├── server/
│   ├── main.c                (200 lines)
│   ├── http/                 (500 lines)
│   ├── tcp/                  (800 lines)
│   ├── game/                 (600 lines)
│   └── utils/                (400 lines)
├── client_proxy/
│   ├── main.c                (150 lines)
│   ├── threads/              (800 lines)
│   └── utils/                (250 lines)
└── webui/
    ├── index.html            (200 lines)
    ├── style.css             (400 lines)
    └── *.js                  (600 lines)
```

### APPENDIX B: Performance Metrics

```
Latency (Local):      10-20ms
Latency (LAN):        20-50ms
Throughput:           100+ messages/sec
Max Players/Room:     5
Max Concurrent Rooms: 100
Memory per Room:      ~50KB
CPU Usage:            <5% (idle)
```

### APPENDIX C: Message Protocol Details

```
Full list of 30 message types...
(See TECHNICAL_REPORT.md Section 6.1)
```

---

## 🎬 PRESENTATION TIPS

1. **Practice timing:** Aim for 15-20 minutes
2. **Have backup:** Screenshots if demo fails
3. **Explain visuals:** Don't just read slides
4. **Engage audience:** Ask questions
5. **Demo early:** Show game in action
6. **Be confident:** You built this!

---

## 🎯 KEY MESSAGES TO EMPHASIZE

1. **Low-level networking mastery**
   - Berkeley sockets, threading, protocols

2. **Real-world problem solving**
   - Race conditions, synchronization, cross-platform

3. **Modern architecture**
   - Multi-threaded, scalable, maintainable

4. **Complete product**
   - Playable game with polished UI

5. **Learning journey**
   - Challenges overcome, skills gained

---

**END OF PRESENTATION GUIDE**

Good luck with your presentation! 🎉
