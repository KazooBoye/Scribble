// WebSocket connection manager
class WSConnection {
    constructor() {
        this.ws = null;
        this.connected = false;
        this.reconnectAttempts = 0;
        this.maxReconnectAttempts = 5;
        this.messageHandlers = {};
    }
    
    connect(url) {
        return new Promise((resolve, reject) => {
            try {
                console.log('[WS] Connecting to', url);
                this.ws = new WebSocket(url);
                
                // Set a timeout for connection
                const timeout = setTimeout(() => {
                    if (!this.connected) {
                        console.error('[WS] Connection timeout');
                        reject(new Error('Connection timeout'));
                        if (this.ws) {
                            this.ws.close();
                        }
                    }
                }, 5000);
                
                this.ws.onopen = () => {
                    console.log('[WS] Connected successfully');
                    clearTimeout(timeout);
                    this.connected = true;
                    this.reconnectAttempts = 0;
                    resolve();
                };
                
                this.ws.onmessage = (event) => {
                    console.log('[WS] Received:', event.data);
                    try {
                        const data = JSON.parse(event.data);
                        if (data.type === 100) {
                            console.log('[WS] Stroke message received, data:', data.data);
                        }
                        this.handleMessage(data);
                    } catch (e) {
                        console.error('[WS] Failed to parse message:', e, event.data);
                    }
                };
                
                this.ws.onerror = (error) => {
                    console.error('[WS] Error:', error);
                    clearTimeout(timeout);
                    reject(error);
                };
                
                this.ws.onclose = (event) => {
                    console.log('[WS] Disconnected', event.code, event.reason);
                    clearTimeout(timeout);
                    this.connected = false;
                    if (event.wasClean) {
                        this.handleDisconnect();
                    }
                };
                
            } catch (e) {
                console.error('[WS] Exception:', e);
                reject(e);
            }
        });
    }
    
    send(type, data) {
        if (!this.connected || !this.ws) {
            console.error('[WS] Not connected');
            return false;
        }
        
        const message = {
            type: type,
            data: data
        };
        
        try {
            this.ws.send(JSON.stringify(message));
            return true;
        } catch (e) {
            console.error('[WS] Send failed:', e);
            return false;
        }
    }
    
    on(type, handler) {
        this.messageHandlers[type] = handler;
    }
    
    handleMessage(message) {
        const handler = this.messageHandlers[message.type];
        if (handler) {
            handler(message.data);
        } else {
            console.log('[WS] Unhandled message type:', message.type);
        }
    }
    
    handleDisconnect() {
        if (this.reconnectAttempts < this.maxReconnectAttempts) {
            console.log('[WS] Attempting to reconnect...');
            this.reconnectAttempts++;
            setTimeout(() => {
                this.connect(this.ws.url);
            }, 2000 * this.reconnectAttempts);
        } else {
            console.log('[WS] Max reconnect attempts reached');
            if (window.game) {
                window.game.showReconnectDialog();
            }
        }
    }
    
    disconnect() {
        if (this.ws) {
            this.ws.close();
            this.ws = null;
        }
        this.connected = false;
    }
}

// Message types (matching protocol.h)
const MSG_TYPE = {
    PING: 0,
    PONG: 1,
    REGISTER: 2,
    REGISTER_ACK: 3,
    JOIN_ROOM: 4,
    CREATE_ROOM: 5,
    ROOM_CREATED: 6,
    ROOM_JOINED: 7,
    ROOM_FULL: 8,
    ROOM_NOT_FOUND: 9,
    GAME_START: 10,
    YOUR_TURN: 11,
    WORD_TO_DRAW: 12,
    ROUND_START: 13,
    CHAT: 14,
    CHAT_BROADCAST: 15,
    GUESS_CORRECT: 16,
    GUESS_WRONG: 17,
    TIMER_UPDATE: 18,
    COUNTDOWN_UPDATE: 19,
    ROUND_END: 20,
    GAME_END: 21,
    PLAYER_JOIN: 22,
    PLAYER_LEAVE: 23,
    SCORE_UPDATE: 24,
    RECONNECT_REQUEST: 25,
    RECONNECT_SUCCESS: 26,
    RECONNECT_FAIL: 27,
    ERROR: 28,
    DISCONNECT: 29
};

const UDP_TYPE = {
    STROKE: 100,
    CLEAR_CANVAS: 101,
    UNDO: 102
};
