# Scribble Game - Makefile
# Multiplayer Drawing & Guessing Game

CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread -std=c11
LDFLAGS = -pthread -lm

# Directories
SERVER_DIR = server
CLIENT_DIR = client_proxy
WEBUI_DIR = webui
BUILD_DIR = build

# Server source files
SERVER_SRCS = \
	$(SERVER_DIR)/main.c \
	$(SERVER_DIR)/http/http_server.c \
	$(SERVER_DIR)/http/router.c \
	$(SERVER_DIR)/http/mime.c \
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

# Client proxy source files
CLIENT_SRCS = \
	$(CLIENT_DIR)/main.c \
	$(CLIENT_DIR)/threads/dispatcher.c \
	$(CLIENT_DIR)/threads/ws_thread.c \
	$(CLIENT_DIR)/threads/tcp_thread.c \
	$(CLIENT_DIR)/threads/udp_thread.c \
	$(CLIENT_DIR)/utils/queue.c \
	$(CLIENT_DIR)/utils/state_cache.c \
	$(CLIENT_DIR)/utils/json.c

# Object files
SERVER_OBJS = $(SERVER_SRCS:$(SERVER_DIR)/%.c=$(BUILD_DIR)/server/%.o)
CLIENT_OBJS = $(CLIENT_SRCS:$(CLIENT_DIR)/%.c=$(BUILD_DIR)/client/%.o)

# Executables
SERVER_BIN = $(BUILD_DIR)/scribble_server
CLIENT_BIN = $(BUILD_DIR)/scribble_proxy

# Targets
.PHONY: all clean server client run-server run-client setup help

all: setup server client
	@echo "╔══════════════════════════════════════════╗"
	@echo "║     Build completed successfully!        ║"
	@echo "╚══════════════════════════════════════════╝"
	@echo ""
	@echo "Server binary: $(SERVER_BIN)"
	@echo "Client binary: $(CLIENT_BIN)"
	@echo ""
	@echo "Run 'make run' to start the game"

# Setup build directories
setup:
	@mkdir -p $(BUILD_DIR)/server/http
	@mkdir -p $(BUILD_DIR)/server/tcp
	@mkdir -p $(BUILD_DIR)/server/udp
	@mkdir -p $(BUILD_DIR)/server/game
	@mkdir -p $(BUILD_DIR)/server/utils
	@mkdir -p $(BUILD_DIR)/client/threads
	@mkdir -p $(BUILD_DIR)/client/utils
	@echo "[SETUP] Build directories created"

# Build server
server: $(SERVER_BIN)

$(SERVER_BIN): $(SERVER_OBJS)
	@echo "[LINK] Linking server executable..."
	@$(CC) $(SERVER_OBJS) -o $@ $(LDFLAGS)
	@echo "[BUILD] Server built: $@"

# Build client proxy
client: $(CLIENT_BIN)

$(CLIENT_BIN): $(CLIENT_OBJS)
	@echo "[LINK] Linking client proxy executable..."
	@$(CC) $(CLIENT_OBJS) -o $@ $(LDFLAGS)
	@echo "[BUILD] Client proxy built: $@"

# Compile server object files
$(BUILD_DIR)/server/%.o: $(SERVER_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile client proxy object files
$(BUILD_DIR)/client/%.o: $(CLIENT_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Run server
run-server: server
	@echo "[RUN] Starting game server..."
	@cd $(BUILD_DIR) && ./scribble_server

# Run client proxy
run-client: client
	@echo "[RUN] Starting client proxy..."
	@cd $(BUILD_DIR) && ./scribble_proxy

# Run both (in separate terminals)
run: all
	@echo "╔══════════════════════════════════════════╗"
	@echo "║         Starting Scribble Game           ║"
	@echo "╚══════════════════════════════════════════╝"
	@echo ""
	@echo "1. Starting server in background..."
	@cd $(BUILD_DIR) && ./scribble_server > server.log 2>&1 &
	@sleep 2
	@echo "2. Starting client proxy in background..."
	@cd $(BUILD_DIR) && ./scribble_proxy > proxy.log 2>&1 &
	@sleep 1
	@echo ""
	@echo "✓ Server running on:"
	@echo "  - HTTP: http://localhost:8080"
	@echo "  - TCP:  localhost:9090"
	@echo "  - UDP:  localhost:9091"
	@echo ""
	@echo "✓ Client proxy running on:"
	@echo "  - WebSocket: ws://localhost:8081"
	@echo ""
	@echo "Open http://localhost:8080 in your browser to play!"
	@echo ""
	@echo "To stop: make stop"
	@echo "To view logs: tail -f build/server.log build/proxy.log"

# Stop all processes
stop:
	@echo "[STOP] Stopping all processes..."
	@pkill -f scribble_server || true
	@pkill -f scribble_proxy || true
	@echo "[STOP] All processes stopped"

# Clean build files
clean:
	@echo "[CLEAN] Removing build directory..."
	@rm -rf $(BUILD_DIR)
	@echo "[CLEAN] Clean complete"

# Install (copy files to build directory)
install: all
	@echo "[INSTALL] Copying webui files..."
	@mkdir -p $(BUILD_DIR)/webui
	@cp -r $(WEBUI_DIR)/* $(BUILD_DIR)/webui/
	@mkdir -p $(BUILD_DIR)/server/game
	@cp $(SERVER_DIR)/game/wordlist.txt $(BUILD_DIR)/server/game/
	@echo "[INSTALL] Installation complete"

# Development - rebuild and run
dev: clean all install run

# Help
help:
	@echo "╔══════════════════════════════════════════╗"
	@echo "║      Scribble Game - Build System        ║"
	@echo "╚══════════════════════════════════════════╝"
	@echo ""
	@echo "Available targets:"
	@echo "  make all         - Build server and client"
	@echo "  make server      - Build server only"
	@echo "  make client      - Build client proxy only"
	@echo "  make run         - Build and run everything"
	@echo "  make run-server  - Run server only"
	@echo "  make run-client  - Run client proxy only"
	@echo "  make install     - Copy resources to build directory"
	@echo "  make clean       - Remove build files"
	@echo "  make stop        - Stop all running processes"
	@echo "  make dev         - Clean, build, and run (development)"
	@echo "  make help        - Show this help"
	@echo ""

# Default target
.DEFAULT_GOAL := help
