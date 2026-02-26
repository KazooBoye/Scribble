# Scribble

A multiplayer drawing and guessing game with real-time networking. Players take turns drawing words while others try to guess them.

## Architecture

- **Server**: C server using POSIX threads, Berkeley sockets, and TCP for real-time game state synchronization
- **Client Library**: C networking library compiled to shared library (dylib/so)
- **Client UI**: Python/Pygame application using ctypes to interface with C library
- **Protocol**: JSON over TCP with 4-byte length prefix framing

## Features

- Real-time drawing synchronization
- Automatic matchmaking and private rooms
- Host controls (kick players, early start)
- Player statistics and leaderboards
- Connection monitoring with PING/PONG
- Reconnection support

## Requirements

### Server
- GCC or Clang
- POSIX-compliant system (Linux, macOS, WSL)
- pthread support

### Client
- Python 3.7+
- Pygame

## Building

Build everything:
```bash
make all
```

Build server only:
```bash
make server
```

Build client library only:
```bash
make client
```

## Running

Start the server:
```bash
make run-server
```

The server listens on port 9090 by default.

Start a client:
```bash
cd client_pygame
python3 main.py
```

## Project Structure

```
server/              # C game server
  tcp/               # TCP server and message handling
  game/              # Game logic, matchmaking, statistics
  utils/             # JSON parser, logger, timer
client_c/            # C networking library source
client_pygame/       # Python/Pygame client UI
build/               # Compiled binaries and objects
```

## Protocol

The game uses a custom JSON-based protocol over TCP. Each message has:
- 4-byte length prefix (network byte order)
- JSON payload with message type and data

Message types include: JOIN, LEAVE, DRAW, CHAT, GUESS, GAME_START, ROUND_START, ROUND_END, etc.

## Game Modes

- **Public rooms**: Automatic matchmaking with other players
- **Private rooms**: Create or join with a 6-character room code

## Statistics

Player statistics are persisted to CSV and include:
- Games played
- Rounds won
- Total score
- Best game score

## Development

Clean build artifacts:
```bash
make clean
```

View available commands:
```bash
make help
```
