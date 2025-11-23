#!/bin/bash

# Scribble Game - Test & Demo Script

set -e

PROJECT_DIR="/Users/caoducanh/Coding/NetworkProgramming/Scribble"
BUILD_DIR="$PROJECT_DIR/build"

echo "╔════════════════════════════════════════════════════╗"
echo "║   Scribble Game - Testing & Demo Script           ║"
echo "╚════════════════════════════════════════════════════╝"
echo ""

# Function to cleanup
cleanup() {
    echo ""
    echo "[CLEANUP] Stopping all processes..."
    pkill -f scribble_server || true
    pkill -f scribble_proxy || true
    echo "[CLEANUP] Done"
}

# Trap exit
trap cleanup EXIT

# Step 1: Build
echo "[STEP 1] Building project..."
cd "$PROJECT_DIR"
make clean > /dev/null 2>&1
make all > /dev/null 2>&1
make install > /dev/null 2>&1
echo "✓ Build completed"
echo ""

# Step 2: Start server
echo "[STEP 2] Starting game server..."
cd "$BUILD_DIR"
./scribble_server > server.log 2>&1 &
SERVER_PID=$!
sleep 2

if ps -p $SERVER_PID > /dev/null; then
    echo "✓ Server started (PID: $SERVER_PID)"
else
    echo "✗ Server failed to start"
    exit 1
fi
echo ""

# Step 3: Start proxy
echo "[STEP 3] Starting client proxy..."
./scribble_proxy > proxy.log 2>&1 &
PROXY_PID=$!
sleep 2

if ps -p $PROXY_PID > /dev/null; then
    echo "✓ Proxy started (PID: $PROXY_PID)"
else
    echo "✗ Proxy failed to start"
    exit 1
fi
echo ""

# Step 4: Check ports
echo "[STEP 4] Checking network ports..."
echo ""
echo "HTTP Server (Web UI):"
lsof -i :8080 | grep LISTEN || echo "  ✗ Port 8080 not listening"

echo ""
echo "TCP Server (Game Logic):"
lsof -i :9090 | grep LISTEN || echo "  ✗ Port 9090 not listening"

echo ""
echo "UDP Server (Drawing):"
lsof -i :9091 || echo "  ✗ Port 9091 not active"

echo ""
echo "WebSocket Proxy:"
lsof -i :8081 | grep LISTEN || echo "  ✗ Port 8081 not listening"

echo ""
echo ""

# Step 5: Display info
echo "╔════════════════════════════════════════════════════╗"
echo "║             🎉 System Ready!                       ║"
echo "╚════════════════════════════════════════════════════╝"
echo ""
echo "Game URL: http://localhost:8080"
echo ""
echo "Components Running:"
echo "  • Server PID:    $SERVER_PID"
echo "  • Proxy PID:     $PROXY_PID"
echo ""
echo "Architecture:"
echo "  • HTTP:          localhost:8080 (Web UI)"
echo "  • TCP:           localhost:9090 (Game Server)"
echo "  • UDP:           localhost:9091 (Drawing)"
echo "  • WebSocket:     localhost:8081 (Proxy)"
echo ""
echo "Multi-threaded Proxy:"
echo "  • Thread 1: WebSocket Listener"
echo "  • Thread 2: TCP Handler"
echo "  • Thread 3: UDP Handler"
echo "  • Thread 4: Dispatcher (Router)"
echo ""
echo "Logs:"
echo "  • Server: $BUILD_DIR/server.log"
echo "  • Proxy:  $BUILD_DIR/proxy.log"
echo ""
echo "Commands:"
echo "  • View logs:  tail -f $BUILD_DIR/*.log"
echo "  • Stop:       make stop"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Open http://localhost:8080 in your browser to play!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Press Ctrl+C to stop all services..."
echo ""

# Keep running
wait
