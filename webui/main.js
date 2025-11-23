// Main game controller
class Game {
    constructor() {
        this.ws = new WSConnection();
        this.canvas = new DrawingCanvas('drawing-canvas');
        this.playerId = null;
        this.username = 'Player';
        this.sessionToken = null;
        this.roomId = null;
        this.players = [];
        this.isDrawing = false;
        
        this.setupUI();
        this.setupMessageHandlers();
    }
    
    async init() {
        const statusEl = document.getElementById('connection-status');
        try {
            statusEl.textContent = 'Connecting to server...';
            
            // Connect to WebSocket proxy - use same host as HTTP
            const wsHost = window.location.hostname;
            const wsUrl = `ws://${wsHost}:8081`;
            
            await this.ws.connect(wsUrl);
            console.log('[GAME] Connected to proxy at', wsUrl);
            
            statusEl.textContent = '✓ Connected to server';
            statusEl.classList.add('connected');
            
            // Start ping/pong heartbeat
            this.startHeartbeat();
            
            // Enable buttons once connected
            document.getElementById('btn-play-now').disabled = false;
            document.getElementById('btn-create-room').disabled = false;
            document.getElementById('btn-join-room').disabled = false;
            
        } catch (e) {
            console.error('[GAME] Failed to connect:', e);
            statusEl.textContent = '✗ Connection failed - Please refresh';
            statusEl.style.background = '#f8d7da';
            statusEl.style.color = '#721c24';
        }
    }
    
    setupUI() {
        // Landing page buttons - disable until connected
        document.getElementById('btn-play-now').disabled = true;
        document.getElementById('btn-create-room').disabled = true;
        document.getElementById('btn-join-room').disabled = true;
        
        document.getElementById('btn-play-now').onclick = () => this.playNow();
        document.getElementById('btn-create-room').onclick = () => this.createRoom();
        document.getElementById('btn-join-room').onclick = () => this.joinRoom();
        
        // Game page buttons
        document.getElementById('btn-send-chat').onclick = () => this.sendChat();
        document.getElementById('chat-input').onkeypress = (e) => {
            if (e.key === 'Enter') this.sendChat();
        };
        
        document.getElementById('btn-clear').onclick = () => this.clearCanvas();
        
        // Drawing tools
        document.getElementById('color-picker').onchange = (e) => {
            this.canvas.setColor(e.target.value);
        };
        
        document.getElementById('brush-size').oninput = (e) => {
            this.canvas.setLineWidth(e.target.value);
            document.getElementById('brush-size-value').textContent = e.target.value;
        };
        
        // Reconnect dialog
        document.getElementById('btn-reconnect').onclick = () => this.reconnect();
        document.getElementById('btn-leave').onclick = () => this.leaveGame();
        
        // Game end dialog
        document.getElementById('btn-play-again').onclick = () => this.playAgain();
    }
    
    setupMessageHandlers() {
        this.ws.on(MSG_TYPE.REGISTER_ACK, (data) => this.handleRegisterAck(data));
        this.ws.on(MSG_TYPE.ROOM_CREATED, (data) => this.handleRoomCreated(data));
        this.ws.on(MSG_TYPE.ROOM_JOINED, (data) => this.handleRoomJoined(data));
        this.ws.on(MSG_TYPE.GAME_START, (data) => this.handleGameStart(data));
        this.ws.on(MSG_TYPE.WORD_TO_DRAW, (data) => this.handleWordToDraw(data));
        this.ws.on(MSG_TYPE.ROUND_START, (data) => this.handleRoundStart(data));
        this.ws.on(MSG_TYPE.ROUND_END, (data) => this.handleRoundEnd(data));
        this.ws.on(MSG_TYPE.CHAT_BROADCAST, (data) => this.handleChatBroadcast(data));
        this.ws.on(MSG_TYPE.GUESS_CORRECT, (data) => this.handleGuessCorrect(data));
        this.ws.on(MSG_TYPE.TIMER_UPDATE, (data) => this.handleTimerUpdate(data));
        this.ws.on(MSG_TYPE.PLAYER_JOIN, (data) => this.handlePlayerJoin(data));
        this.ws.on(MSG_TYPE.PLAYER_LEAVE, (data) => this.handlePlayerLeave(data));
        this.ws.on(MSG_TYPE.SCORE_UPDATE, (data) => this.handleScoreUpdate(data));
        this.ws.on(MSG_TYPE.GAME_END, (data) => this.handleGameEnd(data));
        this.ws.on(MSG_TYPE.RECONNECT_SUCCESS, (data) => this.handleReconnectSuccess(data));
        this.ws.on(UDP_TYPE.CLEAR_CANVAS, (data) => this.handleClearCanvas(data));
        this.ws.on(MSG_TYPE.ERROR, (data) => this.handleError(data));
        this.ws.on(UDP_TYPE.STROKE, (data) => this.handleStroke(data));
    }
    
    register() {
        if (!this.ws.connected) {
            alert('Not connected to server. Please wait...');
            return false;
        }
        this.username = document.getElementById('input-username').value || 'Player';
        this.ws.send(MSG_TYPE.REGISTER, { username: this.username });
        return true;
    }
    
    playNow() {
        if (!this.register()) return;
        setTimeout(() => {
            this.ws.send(MSG_TYPE.JOIN_ROOM, {});
        }, 100);
    }
    
    createRoom() {
        if (!this.register()) return;
        setTimeout(() => {
            this.ws.send(MSG_TYPE.CREATE_ROOM, {});
        }, 100);
    }
    
    joinRoom() {
        const roomCode = document.getElementById('input-room-code').value.toUpperCase();
        if (!roomCode || roomCode.length !== 6) {
            alert('Please enter a valid 6-character room code');
            return;
        }
        
        this.register();
        setTimeout(() => {
            this.ws.send(MSG_TYPE.JOIN_ROOM, { room_code: roomCode });
        }, 500);
    }
    
    sendChat() {
        const input = document.getElementById('chat-input');
        const message = input.value.trim();
        
        if (!message) return;
        
        this.ws.send(MSG_TYPE.CHAT, { message: message });
        input.value = '';
    }
    
    clearCanvas() {
        this.canvas.clear();
        // Send clear command to server to broadcast to all players
        if (this.ws.connected) {
            this.ws.send(UDP_TYPE.CLEAR_CANVAS, {});
        }
    }
    
    showScreen(screenId) {
        document.querySelectorAll('.screen').forEach(s => s.classList.add('hidden'));
        document.getElementById(screenId).classList.remove('hidden');
    }
    
    showStatus(message, type = 'info') {
        const statusEl = document.getElementById('status-message');
        statusEl.textContent = message;
        statusEl.className = 'status-message ' + type;
        
        setTimeout(() => {
            statusEl.textContent = '';
            statusEl.className = 'status-message';
        }, 3000);
    }
    
    updatePlayersList() {
        const listEl = document.getElementById('players-list');
        listEl.innerHTML = '';
        
        this.players.forEach(player => {
            const item = document.createElement('div');
            item.className = 'player-item';
            
            if (player.player_id === this.playerId) {
                item.classList.add('me');
            }
            
            if (player.is_drawing) {
                item.classList.add('drawing');
            }
            
            const onlineStatus = player.online !== false ? '🟢' : '⚫';
            
            item.innerHTML = `
                <span class="player-status">${onlineStatus}</span>
                <span class="player-name">${player.username}</span>
                <span class="player-score">${player.score} pts</span>
            `;
            
            listEl.appendChild(item);
        });
    }
    
    addChatMessage(sender, message, type = 'normal') {
        const chatBox = document.getElementById('chat-box');
        const msgEl = document.createElement('div');
        msgEl.className = 'chat-message ' + type;
        
        if (type === 'system') {
            msgEl.textContent = message;
        } else {
            msgEl.innerHTML = `
                <span class="chat-sender">${sender}:</span>
                <span class="chat-text">${message}</span>
            `;
        }
        
        chatBox.appendChild(msgEl);
        chatBox.scrollTop = chatBox.scrollHeight;
    }
    
    startHeartbeat() {
        setInterval(() => {
            if (this.ws.connected) {
                this.ws.send(MSG_TYPE.PING, {});
            }
        }, 10000);  // Every 10 seconds
    }
    
    showReconnectDialog() {
        document.getElementById('reconnect-dialog').classList.remove('hidden');
    }
    
    hideReconnectDialog() {
        document.getElementById('reconnect-dialog').classList.add('hidden');
    }
    
    reconnect() {
        if (this.sessionToken) {
            this.ws.send(MSG_TYPE.RECONNECT_REQUEST, {
                session_token: this.sessionToken
            });
        }
    }
    
    leaveGame() {
        this.ws.send(MSG_TYPE.DISCONNECT, {});
        this.showScreen('landing-page');
        this.hideReconnectDialog();
    }
    
    playAgain() {
        document.getElementById('game-end-dialog').classList.add('hidden');
        this.playNow();
    }
    
    // Message handlers
    handleRegisterAck(data) {
        this.playerId = data.player_id;
        this.sessionToken = data.session_token;
        console.log('[GAME] Registered as player', this.playerId);
    }
    
    handleRoomCreated(data) {
        this.roomId = data.room_id;
        document.getElementById('room-code').textContent = data.room_code;
        this.showScreen('game-page');
        this.showStatus(`Room created! Code: ${data.room_code}`, 'info');
        this.addChatMessage('System', `Waiting for players... Share code: ${data.room_code}`, 'system');
    }
    
    handleRoomJoined(data) {
        const isInitialJoin = !this.roomId;
        
        console.log('[GAME] handleRoomJoined: data=', data, 'isInitialJoin=', isInitialJoin, 'current roomId=', this.roomId);
        
        this.roomId = data.room_id;
        this.players = data.players || [];
        
        if (isInitialJoin) {
            this.showScreen('game-page');
            
            if (data.room_code) {
                document.getElementById('room-code').textContent = data.room_code;
            }
            
            this.showStatus('Joined room! Waiting for more players...', 'info');
            this.addChatMessage('System', 'You joined the room', 'system');
        }
        
        this.updatePlayersList();
    }
    
    handleGameStart(data) {
        console.log('[GAME] Game started with data:', data);
        this.players = data.players || [];
        this.updatePlayersList();
        this.canvas.clear();
        
        // Update game state from data
        if (data.word_mask) {
            document.getElementById('word-mask').textContent = data.word_mask;
        }
        if (data.round !== undefined) {
            document.getElementById('round-number').textContent = data.round;
        }
        if (data.time_remaining !== undefined) {
            this.startTimer(data.time_remaining);
        }
        
        // Check if current player is drawing
        const currentPlayer = this.players.find(p => p.player_id === this.playerId);
        if (currentPlayer && currentPlayer.is_drawing) {
            this.isDrawing = true;
            this.canvas.enable(true);
            document.getElementById('drawing-tools').classList.remove('hidden');
            this.showStatus('Your turn to draw!', 'info');
        } else {
            this.isDrawing = false;
            this.canvas.enable(false);
            document.getElementById('drawing-tools').classList.add('hidden');
            this.showStatus('Game started! Guess the word!', 'info');
        }
        
        this.addChatMessage('System', 'Game started! Good luck!', 'system');
    }
    
    handleWordToDraw(data) {
        this.isDrawing = true;
        this.canvas.enable(true);
        document.getElementById('drawing-tools').classList.remove('hidden');
        document.getElementById('word-mask').textContent = data.word;
        this.showStatus('Your turn to draw!', 'info');
        this.addChatMessage('System', `Your word is: ${data.word}`, 'system');
    }
    
    handleRoundStart(data) {
        console.log('[GAME] Round started with data:', data);
        console.log('[GAME] My player_id:', this.playerId);
        this.canvas.clear();
        this.players = data.players || this.players;
        this.updatePlayersList();
        
        if (data.word_mask) {
            document.getElementById('word-mask').textContent = data.word_mask;
        }
        if (data.round !== undefined) {
            document.getElementById('round-number').textContent = data.round;
        }
        if (data.time_remaining !== undefined) {
            this.startTimer(data.time_remaining);
        }
        
        // Check if current player is drawing in new round
        const currentPlayer = this.players.find(p => p.player_id === this.playerId);
        console.log('[GAME] Current player in new round:', currentPlayer);
        
        if (currentPlayer && currentPlayer.is_drawing) {
            console.log('[GAME] I AM the drawer for this round');
            this.isDrawing = true;
            this.canvas.enable(true);
            document.getElementById('drawing-tools').classList.remove('hidden');
            this.showStatus('Your turn to draw!', 'info');
        } else {
            console.log('[GAME] I am NOT the drawer for this round');
            this.isDrawing = false;
            this.canvas.enable(false);
            document.getElementById('drawing-tools').classList.add('hidden');
            this.showStatus('New round! Guess the word!', 'info');
        }
        
        this.addChatMessage('System', 'New round started!', 'system');
    }
    
    handleChatBroadcast(data) {
        this.addChatMessage(data.username, data.message, 'normal');
    }
    
    handleRoundEnd(data) {
        console.log('[GAME] Round ended');
        this.addChatMessage('System', 'Round ended!', 'system');
        // Update players and scores
        if (data.players) {
            this.players = data.players;
            this.updatePlayersList();
        }
    }
    
    handleGuessCorrect(data) {
        this.addChatMessage('System', `${data.username} guessed correctly!`, 'correct');
        // Refresh scores
        if (data.player_id && data.score !== undefined) {
            const player = this.players.find(p => p.player_id === data.player_id);
            if (player) {
                player.score = data.score;
                this.updatePlayersList();
            }
        }
    }
    
    handleTimerUpdate(data) {
        if (data.time_remaining !== undefined) {
            // Just update the display, don't restart client-side timer
            document.getElementById('timer').textContent = data.time_remaining;
        }
    }
    
    startTimer(seconds) {
        // Clear any existing timer
        if (this.timerInterval) {
            clearInterval(this.timerInterval);
            this.timerInterval = null;
        }
        
        // Just display the initial time, server will send updates
        document.getElementById('timer').textContent = seconds;
    }
    
    handlePlayerJoin(data) {
        this.addChatMessage('System', `${data.username} joined`, 'system');
        // Refresh player list
    }
    
    handlePlayerLeave(data) {
        this.addChatMessage('System', `${data.username} left`, 'system');
        // Mark player as offline
        const player = this.players.find(p => p.player_id === data.player_id);
        if (player) {
            player.online = false;
            this.updatePlayersList();
        }
    }
    
    handleScoreUpdate(data) {
        console.log('[GAME] Score update:', data);
        // Update players with new data
        if (data.players) {
            this.players = data.players;
        } else if (data.player_id !== undefined && data.score !== undefined) {
            // Single player score update
            const player = this.players.find(p => p.player_id === data.player_id);
            if (player) {
                player.score = data.score;
            }
        }
        this.updatePlayersList();
    }
    
    handleGameEnd(data) {
        const dialog = document.getElementById('game-end-dialog');
        const scoresEl = document.getElementById('final-scores');
        
        scoresEl.innerHTML = '';
        
        if (data.players) {
            const sorted = data.players.sort((a, b) => b.score - a.score);
            sorted.forEach((player, idx) => {
                const item = document.createElement('div');
                item.className = 'score-item';
                if (idx === 0) item.classList.add('winner');
                
                item.innerHTML = `
                    <span>${idx + 1}. ${player.username}</span>
                    <span>${player.score} pts</span>
                `;
                
                scoresEl.appendChild(item);
            });
        }
        
        dialog.classList.remove('hidden');
    }
    
    handleReconnectSuccess(data) {
        this.hideReconnectDialog();
        this.showStatus('Reconnected successfully!', 'info');
        this.handleRoomJoined(data);
    }
    
    handleError(data) {
        alert('Error: ' + (data.error || 'Unknown error'));
    }
    
    handleClearCanvas(data) {
        console.log('[GAME] Clear canvas received');
        this.canvas.clear();
    }
    
    handleStroke(data) {
        console.log('[GAME] Received stroke:', data, 'isDrawing:', this.isDrawing);
        // Draw stroke received from other players
        // Only draw if we're not the drawer (strokes are only broadcast to others)
        if (!this.isDrawing) {
            console.log('[GAME] Drawing stroke on canvas');
            this.canvas.drawStroke(data);
        } else {
            console.log('[GAME] Ignoring stroke - I am the drawer');
        }
    }
}

// Initialize game when page loads
window.addEventListener('DOMContentLoaded', async () => {
    window.game = new Game();
    await window.game.init();
});

// Warn user before closing/refreshing during active game
window.addEventListener('beforeunload', (e) => {
    if (window.game && window.game.playerId && window.game.ws && window.game.ws.ws) {
        e.preventDefault();
        e.returnValue = 'You are currently in a game. Leaving will disconnect you. Are you sure?';
        return e.returnValue;
    }
});
