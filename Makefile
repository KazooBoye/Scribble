# Scribble Game - Makefile
# Multiplayer Drawing & Guessing Game

CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread -std=c11 -D_GNU_SOURCE
LDFLAGS = -pthread -lm

# Detect OS for platform-specific libraries
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS
    CLIENT_LIB = libscribble_client.dylib
    CLIENT_LIB_FLAGS = -dynamiclib
else
    # Linux/WSL
    CLIENT_LIB = libscribble_client.so
    CLIENT_LIB_FLAGS = -shared -fPIC
endif

# Directories
SERVER_DIR = server
CLIENT_C_DIR = client_c
PYGAME_DIR = client_pygame
BUILD_DIR = build
LOGS_DIR = logs

# Server source files
SERVER_SRCS = \
	$(SERVER_DIR)/main.c \
	$(SERVER_DIR)/tcp/tcp_server.c \
	$(SERVER_DIR)/tcp/tcp_handler.c \
	$(SERVER_DIR)/tcp/tcp_parser.c \
	$(SERVER_DIR)/udp/udp_server.c \
	$(SERVER_DIR)/udp/udp_broadcast.c \
	$(SERVER_DIR)/game/game_logic.c \
	$(SERVER_DIR)/game/matchmaking.c \
	$(SERVER_DIR)/game/reconnection.c \
	$(SERVER_DIR)/utils/logger.c \
	$(SERVER_DIR)/utils/json.c \
	$(SERVER_DIR)/utils/timer.c

# Object files
SERVER_OBJS = $(SERVER_SRCS:$(SERVER_DIR)/%.c=$(BUILD_DIR)/server/%.o)

# Executables
SERVER_BIN = $(BUILD_DIR)/scribble_server
CLIENT_LIB_PATH = $(BUILD_DIR)/$(CLIENT_LIB)

# Targets
.PHONY: all clean server client run-server setup help stop

all: setup server client install
	@echo "╔══════════════════════════════════════════╗"
	@echo "║     Build completed successfully!        ║"
	@echo "╚══════════════════════════════════════════╝"
	@echo ""
	@echo "Server binary: $(SERVER_BIN)"
	@echo "Client library: $(CLIENT_LIB_PATH)"
	@echo ""
	@echo "Run 'make run-server' to start the server"
	@echo "Run 'python3 client_pygame/main.py' to launch the Pygame client"

# Setup build directories
setup:
	@mkdir -p $(BUILD_DIR)/server/http
	@mkdir -p $(BUILD_DIR)/server/tcp
	@mkdir -p $(BUILD_DIR)/server/udp
	@mkdir -p $(BUILD_DIR)/server/game
	@mkdir -p $(BUILD_DIR)/server/utils
	@echo "[SETUP] Build directories created"

# Build server
server: $(SERVER_BIN)

$(SERVER_BIN): $(SERVER_OBJS)
	@echo "[LINK] Linking server executable..."
	@$(CC) $(SERVER_OBJS) -o $@ $(LDFLAGS)
	@echo "[BUILD] Server built: $@"

# Build client C library (shared library for Python ctypes)
client: $(CLIENT_LIB_PATH)

$(CLIENT_LIB_PATH): $(CLIENT_C_DIR)/network.c $(CLIENT_C_DIR)/network.h
	@echo "[CC] Building client networking library..."
	@$(CC) $(CFLAGS) $(CLIENT_LIB_FLAGS) -o $@ $(CLIENT_C_DIR)/network.c $(LDFLAGS)
	@echo "[BUILD] Client library built: $@"

# Compile server object files
$(BUILD_DIR)/server/%.o: $(SERVER_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Run server
run-server: server
	@echo "[RUN] Starting game server..."
	@mkdir -p $(LOGS_DIR)
	@cd $(BUILD_DIR) && ./scribble_server 2>&1 | tee ../$(LOGS_DIR)/server.log

# Run client (Python + Pygame with C library)
# Usage: make run-client [HOST=192.168.1.100]
run-client: client
	@echo "[RUN] Starting Pygame client..."
	@python3 client_pygame/main.py $(if $(HOST),--host $(HOST),)

# Run both (in separate terminals)
run: all
	@echo "╔══════════════════════════════════════════╗"
	@echo "║         Starting Scribble Game           ║"
	@echo "╚══════════════════════════════════════════╝"
	@echo ""
	@mkdir -p $(LOGS_DIR)
	@echo "Starting server in background..."
	@cd $(BUILD_DIR) && ./scribble_server > ../$(LOGS_DIR)/server.log 2>&1 &
	@sleep 2
	@echo ""
	@echo "✓ Server running on:"
	@echo "  - TCP:  localhost:9090"
	@echo "  - UDP:  localhost:9091"
	@echo ""
	@echo "✓ To start client: make run-client"
	@echo "  Or run: python3 client_pygame/main.py"
	@echo ""
	@echo "To stop: make stop"
	@echo "To view logs: tail -f logs/server.log"

# Stop all processes
stop:
	@echo "[STOP] Stopping all processes..."
	@pkill -f scribble_server || true
	@echo "[STOP] All processes stopped"

# Clean build files
clean:
	@echo "[CLEAN] Removing build directory..."
	@rm -rf $(BUILD_DIR)
	@echo "[CLEAN] Clean complete"

# Clean logs
clean-logs:
	@echo "[CLEAN] Removing logs directory..."
	@rm -rf $(LOGS_DIR)
	@echo "[CLEAN] Logs cleaned"

# Clean everything
clean-all: clean clean-logs
	@echo "[CLEAN] Everything cleaned"

# Install (copy files to build directory)
install:
	@echo "[INSTALL] Copying game resources..."
	@mkdir -p $(BUILD_DIR)/server/game
	@cp $(SERVER_DIR)/game/wordlist.txt $(BUILD_DIR)/server/game/
	@echo "[INSTALL] Installation complete"

# Development - rebuild and run
dev: clean all install run

# View logs
logs:
	@echo "[LOGS] Tailing server logs..."
	@echo "Press Ctrl+C to stop"
	@tail -f $(LOGS_DIR)/server.log

# Help
help:
	@echo "╔══════════════════════════════════════════╗"
	@echo "║      Scribble Game - Build System        ║"
	@echo "╚══════════════════════════════════════════╝"
	@echo ""
	@echo "Available targets:"
	@echo "  make all         - Build server and client library"
	@echo "  make server      - Build server only"
	@echo "  make client      - Build client C library (shared .so/.dylib)"
	@echo "  make run         - Build and run server"
	@echo "  make run-server  - Run server only"
	@echo "  make run-client  - Run Pygame client (requires: make client)"
	@echo "  make install     - Copy resources to build directory"
	@echo "  make logs        - View server logs"
	@echo "  make clean       - Remove build files"
	@echo "  make clean-logs  - Remove log files"
	@echo "  make clean-all   - Remove build and log files"
	@echo "  make stop        - Stop all running processes"
	@echo "  make dev         - Clean, build, and run (development)"
	@echo "  make help        - Show this help"
	@echo ""
	@echo "To play:"
	@echo "  1. make all               # Build everything"
	@echo "  2. make run-server        # Start server in terminal 1"
	@echo "  3. make run-client        # Start Pygame client in terminal 2"
	@echo ""

# Default target
.DEFAULT_GOAL := help
