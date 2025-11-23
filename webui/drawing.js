// Drawing canvas manager
class DrawingCanvas {
    constructor(canvasId) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas.getContext('2d');
        this.isDrawing = false;
        this.lastX = 0;
        this.lastY = 0;
        this.color = '#000000';
        this.lineWidth = 5;
        this.enabled = false;
        this.strokeBuffer = [];
        this.strokeId = 0;
        
        this.setupEventListeners();
        this.clear();
    }
    
    setupEventListeners() {
        this.canvas.addEventListener('mousedown', (e) => this.startDrawing(e));
        this.canvas.addEventListener('mousemove', (e) => this.draw(e));
        this.canvas.addEventListener('mouseup', () => this.stopDrawing());
        this.canvas.addEventListener('mouseout', () => this.stopDrawing());
        
        // Touch events for mobile
        this.canvas.addEventListener('touchstart', (e) => {
            e.preventDefault();
            const touch = e.touches[0];
            const mouseEvent = new MouseEvent('mousedown', {
                clientX: touch.clientX,
                clientY: touch.clientY
            });
            this.canvas.dispatchEvent(mouseEvent);
        });
        
        this.canvas.addEventListener('touchmove', (e) => {
            e.preventDefault();
            const touch = e.touches[0];
            const mouseEvent = new MouseEvent('mousemove', {
                clientX: touch.clientX,
                clientY: touch.clientY
            });
            this.canvas.dispatchEvent(mouseEvent);
        });
        
        this.canvas.addEventListener('touchend', (e) => {
            e.preventDefault();
            this.stopDrawing();
        });
    }
    
    startDrawing(e) {
        if (!this.enabled) return;
        
        this.isDrawing = true;
        const rect = this.canvas.getBoundingClientRect();
        this.lastX = e.clientX - rect.left;
        this.lastY = e.clientY - rect.top;
    }
    
    draw(e) {
        if (!this.isDrawing || !this.enabled) return;
        
        const rect = this.canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;
        
        // Draw locally
        this.drawLine(this.lastX, this.lastY, x, y, this.color, this.lineWidth);
        
        // Send stroke to server via WebSocket
        if (window.game && window.game.ws.connected) {
            const stroke = {
                stroke_id: this.strokeId++,
                x1: this.lastX,
                y1: this.lastY,
                x2: x,
                y2: y,
                color: this.hexToInt(this.color),
                thickness: this.lineWidth,
                timestamp: Date.now()
            };
            
            console.log('[DRAW] Sending stroke:', stroke);
            window.game.ws.send(UDP_TYPE.STROKE, stroke);
        }
        
        this.lastX = x;
        this.lastY = y;
    }
    
    stopDrawing() {
        this.isDrawing = false;
    }
    
    drawLine(x1, y1, x2, y2, color, thickness) {
        this.ctx.strokeStyle = color;
        this.ctx.lineWidth = thickness;
        this.ctx.lineCap = 'round';
        this.ctx.lineJoin = 'round';
        
        this.ctx.beginPath();
        this.ctx.moveTo(x1, y1);
        this.ctx.lineTo(x2, y2);
        this.ctx.stroke();
    }
    
    drawStroke(stroke) {
        const color = this.intToHex(stroke.color);
        this.drawLine(stroke.x1, stroke.y1, stroke.x2, stroke.y2, color, stroke.thickness);
    }
    
    clear() {
        this.ctx.fillStyle = 'white';
        this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
    }
    
    setColor(color) {
        this.color = color;
    }
    
    setLineWidth(width) {
        this.lineWidth = width;
    }
    
    enable(enabled) {
        this.enabled = enabled;
        this.canvas.style.cursor = enabled ? 'crosshair' : 'not-allowed';
    }
    
    hexToInt(hex) {
        return parseInt(hex.replace('#', ''), 16);
    }
    
    intToHex(int) {
        return '#' + int.toString(16).padStart(6, '0');
    }
}
