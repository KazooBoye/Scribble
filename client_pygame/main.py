#!/usr/bin/env python3
"""
Scribble Game - Pygame Client (Redesigned for Pygame)
Clean, simple layout optimized for Pygame rendering
"""

import pygame
import sys
from network_wrapper import NetworkClient
from protocol import MSG_TYPE, COLORS, get_color
from resources import ResourceManager

# Initialize Pygame
pygame.init()

# Window settings
WINDOW_WIDTH = 1200
WINDOW_HEIGHT = 700
FPS = 60

# Layout
SIDEBAR_WIDTH = 200
CANVAS_WIDTH = 700
CANVAS_HEIGHT = 500
CANVAS_X = SIDEBAR_WIDTH + 30
CANVAS_Y = 120

# Colors
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
GRAY = (200, 200, 200)
LIGHT_GRAY = (240, 240, 240)
DARK_GRAY = (100, 100, 100)
BLUE = (70, 130, 220)
GREEN = (76, 175, 80)
RED = (244, 67, 54)
YELLOW = (255, 193, 7)
BG_COLOR = (245, 245, 245)

# Game States
STATE_LANDING = 0
STATE_WAITING = 1
STATE_PLAYING = 2
STATE_ENDED = 3


class Button:
    def __init__(self, x, y, width, height, text, color=BLUE, text_color=WHITE):
        self.rect = pygame.Rect(x, y, width, height)
        self.text = text
        self.color = color
        self.text_color = text_color
        self.hover = False
        
    def draw(self, screen, font):
        color = tuple(min(c + 20, 255) for c in self.color) if self.hover else self.color
        pygame.draw.rect(screen, color, self.rect, border_radius=5)
        pygame.draw.rect(screen, DARK_GRAY, self.rect, 2, border_radius=5)
        
        text_surface = font.render(self.text, True, self.text_color)
        text_rect = text_surface.get_rect(center=self.rect.center)
        screen.blit(text_surface, text_rect)
    
    def handle_event(self, event):
        if event.type == pygame.MOUSEMOTION:
            self.hover = self.rect.collidepoint(event.pos)
        elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            if self.rect.collidepoint(event.pos):
                return True
        return False


class InputBox:
    def __init__(self, x, y, width, height, placeholder='', max_length=20):
        self.rect = pygame.Rect(x, y, width, height)
        self.placeholder = placeholder
        self.text = ''
        self.max_length = max_length
        self.active = False
        
    def draw(self, screen, font):
        color = BLUE if self.active else GRAY
        pygame.draw.rect(screen, WHITE, self.rect)
        pygame.draw.rect(screen, color, self.rect, 2, border_radius=3)
        
        display_text = self.text if self.text else self.placeholder
        text_color = BLACK if self.text else DARK_GRAY
        text_surface = font.render(display_text, True, text_color)
        screen.blit(text_surface, (self.rect.x + 10, self.rect.y + 10))
        
        if self.active and len(self.text) > 0:
            cursor_x = self.rect.x + 10 + font.size(self.text)[0]
            pygame.draw.line(screen, BLACK, 
                           (cursor_x, self.rect.y + 8),
                           (cursor_x, self.rect.y + self.rect.height - 8), 2)
    
    def handle_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            self.active = self.rect.collidepoint(event.pos)
        elif event.type == pygame.KEYDOWN and self.active:
            if event.key == pygame.K_BACKSPACE:
                self.text = self.text[:-1]
            elif len(self.text) < self.max_length and event.unicode.isprintable():
                self.text += event.unicode


class DrawingCanvas:
    def __init__(self, x, y, width, height):
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.surface = pygame.Surface((width, height))
        self.surface.fill(WHITE)
        
        self.drawing = False
        self.last_pos = None
        self.color_index = 0
        self.line_width = 5
        self.enabled = False
        
    def handle_mouse_down(self, pos):
        if self.enabled and self.point_in_canvas(pos):
            self.drawing = True
            canvas_pos = (pos[0] - self.x, pos[1] - self.y)
            self.last_pos = canvas_pos
            
    def handle_mouse_up(self, pos):
        self.drawing = False
        self.last_pos = None
        
    def handle_mouse_move(self, pos, network):
        if self.drawing and self.enabled and self.point_in_canvas(pos):
            canvas_pos = (pos[0] - self.x, pos[1] - self.y)
            
            if self.last_pos:
                color = get_color(self.color_index)
                pygame.draw.line(self.surface, color, self.last_pos, canvas_pos, self.line_width)
                
                network.send_tcp(MSG_TYPE.UDP_STROKE, {
                    'x1': self.last_pos[0],
                    'y1': self.last_pos[1],
                    'x2': canvas_pos[0],
                    'y2': canvas_pos[1],
                    'color': self.color_index,
                    'thickness': self.line_width
                })
            
            self.last_pos = canvas_pos
    
    def point_in_canvas(self, pos):
        return (self.x <= pos[0] <= self.x + self.width and
                self.y <= pos[1] <= self.y + self.height)
    
    def draw_stroke(self, x1, y1, x2, y2, color_index, thickness):
        color = get_color(color_index)
        pygame.draw.line(self.surface, color, (x1, y1), (x2, y2), thickness)
    
    def clear(self):
        self.surface.fill(WHITE)
    
    def set_color(self, index):
        self.color_index = index
    
    def render(self, screen):
        screen.blit(self.surface, (self.x, self.y))
        border_color = GREEN if self.enabled else GRAY
        pygame.draw.rect(screen, border_color, (self.x, self.y, self.width, self.height), 3)


class ScribbleGame:
    def __init__(self, host='localhost', tcp_port=9090):
        self.screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
        pygame.display.set_caption("Scribble - Drawing & Guessing Game")
        self.clock = pygame.time.Clock()
        self.running = True
        
        # Load resources
        self.resources = ResourceManager()
        
        # Fonts
        self.font_large = pygame.font.Font(None, 48)
        self.font_medium = pygame.font.Font(None, 28)
        self.font_small = pygame.font.Font(None, 20)
        
        # Server config
        self.server_host = host
        self.server_tcp_port = tcp_port
        
        # Game state
        self.state = STATE_LANDING
        self.connected = False
        self.reconnecting = False
        self.last_ping_time = pygame.time.get_ticks()
        self.username = ""
        self.player_id = None
        self.session_token = None
        self.room_id = None
        self.room_code = ""
        self.players = []
        self.chat_messages = []
        self.is_drawer = False
        self.word_to_draw = ""
        self.word_mask = "_ _ _ _"
        self.timer = 0
        self.countdown = 0
        self.round_number = 0
        self.total_rounds = 0
        self.status_message = "Connecting..."
        
        # UI Components
        self.canvas = DrawingCanvas(CANVAS_X, CANVAS_Y, CANVAS_WIDTH, CANVAS_HEIGHT)
        self.chat_input = ""
        
        # Landing screen
        center_x = WINDOW_WIDTH // 2
        self.username_input = InputBox(center_x - 200, 250, 400, 40, "Enter your name...", 20)
        self.room_code_input = InputBox(center_x - 200, 450, 200, 40, "ROOM CODE", 6)
        self.btn_play_now = Button(center_x - 200, 320, 400, 50, "Play Now", GREEN)
        self.btn_create_room = Button(center_x - 200, 380, 400, 50, "Create Private Room", BLUE)
        self.btn_join_room = Button(center_x + 10, 450, 190, 40, "Join Room", BLUE)
        self.btn_return_home = Button(center_x - 150, 600, 300, 50, "Return to Home", BLUE)
        
        # Color palette buttons (for game screen)
        self.color_buttons = []
        for i in range(10):
            x = CANVAS_X + 10 + i * 40
            y = CANVAS_Y - 45
            self.color_buttons.append(pygame.Rect(x, y, 35, 35))
        
        # Clear button
        self.btn_clear = Button(CANVAS_X + CANVAS_WIDTH - 120, CANVAS_Y - 45, 110, 35, "Clear", RED, WHITE)
        
        # Networking
        self.network = NetworkClient()
        self.setup_network_handlers()
        
        if self.network.connect(self.server_host, self.server_tcp_port):
            self.connected = True
            self.status_message = f"Connected to {self.server_host}!"
        else:
            self.status_message = f"Failed to connect to {self.server_host}"
    
    def setup_network_handlers(self):
        self.network.register_handler(MSG_TYPE.PING, self.handle_ping)
        self.network.register_handler(MSG_TYPE.REGISTER_ACK, self.handle_register_ack)
        self.network.register_handler(MSG_TYPE.ROOM_CREATED, self.handle_room_created)
        self.network.register_handler(MSG_TYPE.ROOM_JOINED, self.handle_room_joined)
        self.network.register_handler(MSG_TYPE.ROOM_FULL, self.handle_room_full)
        self.network.register_handler(MSG_TYPE.ROOM_NOT_FOUND, self.handle_room_not_found)
        self.network.register_handler(MSG_TYPE.GAME_START, self.handle_game_start)
        self.network.register_handler(MSG_TYPE.ROUND_START, self.handle_round_start)
        self.network.register_handler(MSG_TYPE.YOUR_TURN, self.handle_your_turn)
        self.network.register_handler(MSG_TYPE.WORD_TO_DRAW, self.handle_word_to_draw)
        self.network.register_handler(MSG_TYPE.CHAT, self.handle_chat)  # Handle direct CHAT messages
        self.network.register_handler(MSG_TYPE.CHAT_BROADCAST, self.handle_chat)
        self.network.register_handler(MSG_TYPE.GUESS_CORRECT, self.handle_guess_correct)
        self.network.register_handler(MSG_TYPE.GUESS_WRONG, self.handle_guess_wrong)
        self.network.register_handler(MSG_TYPE.UDP_STROKE, self.handle_draw_stroke)
        self.network.register_handler(MSG_TYPE.UDP_CLEAR_CANVAS, self.handle_clear_canvas)
        self.network.register_handler(MSG_TYPE.TIMER_UPDATE, self.handle_timer)
        self.network.register_handler(MSG_TYPE.COUNTDOWN_UPDATE, self.handle_countdown)
        self.network.register_handler(MSG_TYPE.PLAYER_JOIN, self.handle_player_join)
        self.network.register_handler(MSG_TYPE.PLAYER_LEAVE, self.handle_player_leave)
        self.network.register_handler(MSG_TYPE.ROUND_END, self.handle_round_end)
        self.network.register_handler(MSG_TYPE.GAME_END, self.handle_game_end)
        self.network.register_handler(MSG_TYPE.SCORE_UPDATE, self.handle_score_update)
        self.network.register_handler(MSG_TYPE.RECONNECT_SUCCESS, self.handle_reconnect_success)
        self.network.register_handler(MSG_TYPE.RECONNECT_FAIL, self.handle_reconnect_fail)
        self.network.register_handler(MSG_TYPE.ERROR, self.handle_error)
    
    # Message handlers
    def handle_ping(self, data):
        """Respond to server ping with pong"""
        self.network.send_tcp(MSG_TYPE.PONG, {})
        self.last_ping_time = pygame.time.get_ticks()
    
    def handle_register_ack(self, data):
        self.player_id = data.get('player_id')
        self.session_token = data.get('session_token')
        self.username = data.get('username', self.username)
        print(f"[Game] Registered: {self.username} (ID: {self.player_id})")
    
    def handle_room_created(self, data):
        self.room_id = data.get('room_id')
        self.room_code = data.get('room_code', '')
        self.state = STATE_WAITING
        self.status_message = f"Room created: {self.room_code}"
        print(f"[Game] Created room {self.room_code}")
    
    def handle_room_joined(self, data):
        self.room_id = data.get('room_id')
        self.room_code = data.get('room_code', '')
        self.players = data.get('players', [])
        self.state = STATE_WAITING
        self.status_message = "Waiting for players..."
        print(f"[Game] Joined room, {len(self.players)} players")
    
    def handle_room_full(self, data):
        """Handle room full error"""
        self.status_message = "Room is full!"
        self.state = STATE_LANDING
        self.chat_messages.append("* Room is full, try another one")
        print("[Game] Room full")
    
    def handle_room_not_found(self, data):
        """Handle room not found error"""
        self.status_message = "Room not found!"
        self.state = STATE_LANDING
        self.chat_messages.append("* Room not found, check the code")
        print("[Game] Room not found")
    
    def handle_game_start(self, data):
        self.state = STATE_PLAYING
        self.round_number = data.get('round', 1)
        self.total_rounds = data.get('total_rounds', 3)
        self.word_mask = data.get('word_mask', '_ _ _ _')
        self.players = data.get('players', [])
        self.canvas.clear()
        self.countdown = 0
        self.canvas.enabled = False
        self.is_drawer = False
        self.word_to_draw = ""
        
        # Find if current player is the drawer
        for player_data in self.players:
            if player_data.get('player_id') == self.player_id and player_data.get('is_drawing'):
                self.is_drawer = True
                self.canvas.enabled = True
                if 'word' in data: # Word to draw is only sent to the drawer
                    self.word_to_draw = data['word']
                break
        
        print(f"[Game] Game started - Round {self.round_number}/{self.total_rounds}, is_drawer: {self.is_drawer}, canvas enabled: {self.canvas.enabled}")
    
    def handle_round_start(self, data):
        self.round_number = data.get('round', self.round_number)
        self.total_rounds = data.get('total_rounds', self.total_rounds)
        self.word_mask = data.get('word_mask', '_ _ _ _')
        self.players = data.get('players', self.players)  # Update players list
        self.canvas.clear()
        self.canvas.enabled = False
        self.is_drawer = False
        self.word_to_draw = ""
        
        # Find if current player is the drawer
        for player_data in self.players:
            if player_data.get('player_id') == self.player_id and player_data.get('is_drawing'):
                self.is_drawer = True
                self.canvas.enabled = True
                if 'word' in data: # Word to draw is only sent to the drawer
                    self.word_to_draw = data['word']
                break
        
        print(f"[Game] Round {self.round_number} started - is_drawer: {self.is_drawer}, canvas enabled: {self.canvas.enabled}")
    
    def handle_your_turn(self, data):
        self.is_drawer = True
        self.canvas.enabled = True
    
    def handle_word_to_draw(self, data):
        self.word_to_draw = data.get('word', '')
        self.canvas.enabled = True
    
    def handle_chat(self, data):
        username = data.get('username', 'Unknown')
        message = data.get('message', '')
        self.chat_messages.append(f"{username}: {message}")
        if len(self.chat_messages) > 50:
            self.chat_messages.pop(0)
        print(f"[Chat] {username}: {message}")
    
    def handle_draw_stroke(self, data):
        # Server already excludes sender from broadcast, no need to check player_id
        x1 = data.get('x1', 0)
        y1 = data.get('y1', 0)
        x2 = data.get('x2', 0)
        y2 = data.get('y2', 0)
        color_idx = data.get('color', 0)
        thickness = data.get('thickness', 3)
        
        self.canvas.draw_stroke(x1, y1, x2, y2, color_idx, thickness)
        print(f"[Game] Drawing stroke: ({x1:.1f},{y1:.1f}) -> ({x2:.1f},{y2:.1f}) color={color_idx}")
    
    def handle_clear_canvas(self, data):
        self.canvas.clear()
    
    def handle_timer(self, data):
        self.timer = data.get('time_remaining', 0)
    
    def handle_countdown(self, data):
        self.countdown = data.get('countdown', 0)
    
    def handle_player_join(self, data):
        player = data.get('player')
        if player:
            self.players.append(player)
            username = player.get('username', 'Player')
            self.chat_messages.append(f"* {username} joined")
    
    def handle_player_leave(self, data):
        player_id = data.get('player_id')
        self.players = [p for p in self.players if p.get('player_id') != player_id]
        username = data.get('username', 'Player')
        self.chat_messages.append(f"* {username} left")
    
    def handle_round_end(self, data):
        word = data.get('word', '')
        self.players = data.get('players', self.players)
        self.chat_messages.append(f"* Round ended! Word: {word}")
        self.word_to_draw = ""
        self.is_drawer = False
        self.canvas.enabled = False
        print(f"[Game] Round ended - canvas disabled, is_drawer: {self.is_drawer}")
    
    def handle_game_end(self, data):
        self.state = STATE_ENDED
        self.players = data.get('players', self.players)
        self.players.sort(key=lambda p: p.get('score', 0), reverse=True)
        self.canvas.enabled = False
    
    def handle_guess_correct(self, data):
        username = data.get('username', 'Player')
        score = data.get('score', 0)
        self.chat_messages.append(f"* {username} guessed correctly! +{score}")
        
        player_id = data.get('player_id')
        for player in self.players:
            if player.get('player_id') == player_id:
                player['score'] = player.get('score', 0) + score
                break
    
    def handle_guess_wrong(self, data):
        """Handle incorrect guess - just log it"""
        # Server broadcasts wrong guesses as chat, so nothing special to do
        pass
    
    def handle_score_update(self, data):
        """Handle score updates from server"""
        player_id = data.get('player_id')
        new_score = data.get('score', 0)
        
        for player in self.players:
            if player.get('player_id') == player_id:
                player['score'] = new_score
                break
    
    def handle_error(self, data):
        error_msg = data.get('message', 'Error')
        self.status_message = f"Error: {error_msg}"
    
    def handle_reconnect_success(self, data):
        """Handle successful reconnection"""
        print("[Game] Reconnection successful!")
        self.connected = True
        self.status_message = "Reconnected successfully!"
        
        # Restore game state from server
        if 'room_id' in data:
            self.handle_room_joined(data)
        if 'players' in data:
            self.players = data.get('players', [])
        if 'state' in data:
            # Restore game state
            state_str = data.get('state', 'WAITING')
            if state_str == 'PLAYING':
                self.state = STATE_PLAYING
            elif state_str == 'WAITING':
                self.state = STATE_WAITING
        
        print(f"[Game] Game state restored. Room: {self.room_id}, State: {self.state}")
    
    def handle_reconnect_fail(self, data):
        """Handle failed reconnection"""
        error = data.get('error', 'Unknown error')
        print(f"[Game] Reconnection failed: {error}")
        self.status_message = f"Reconnection failed: {error}"
        self.session_token = None  # Clear invalid token
        
        # Return to landing page
        self.state = STATE_LANDING
    
    # User actions
    def register_and_play_now(self):
        if not self.username_input.text.strip():
            self.status_message = "Please enter your name!"
            return
        
        self.username = self.username_input.text.strip()
        self.network.send_tcp(MSG_TYPE.REGISTER, {'username': self.username})
        pygame.time.wait(100)
        self.network.send_tcp(MSG_TYPE.JOIN_ROOM, {'room_id': 0})
        self.status_message = "Finding game..."
    
    def register_and_create_room(self):
        if not self.username_input.text.strip():
            self.status_message = "Please enter your name!"
            return
        
        self.username = self.username_input.text.strip()
        self.network.send_tcp(MSG_TYPE.REGISTER, {'username': self.username})
        pygame.time.wait(100)
        self.network.send_tcp(MSG_TYPE.CREATE_ROOM, {})
        self.status_message = "Creating room..."
    
    def register_and_join_room(self):
        if not self.username_input.text.strip():
            self.status_message = "Please enter your name!"
            return
        
        room_code = self.room_code_input.text.strip().upper()
        if len(room_code) != 6:
            self.status_message = "Room code must be 6 characters!"
            return
        
        self.username = self.username_input.text.strip()
        self.network.send_tcp(MSG_TYPE.REGISTER, {'username': self.username})
        pygame.time.wait(100)
        self.network.send_tcp(MSG_TYPE.JOIN_ROOM, {'room_code': room_code})
        self.status_message = f"Joining {room_code}..."
    
    def send_chat(self):
        if self.chat_input.strip() and self.connected:
            self.network.send_tcp(MSG_TYPE.CHAT, {'message': self.chat_input})
            self.chat_input = ""
    
    def clear_canvas_action(self):
        if self.is_drawer and self.canvas.enabled:
            self.canvas.clear()
            self.network.send_tcp(MSG_TYPE.UDP_CLEAR_CANVAS, {})
    
    def return_to_home(self):
        self.state = STATE_LANDING
        self.room_id = None
        self.players = []
        self.chat_messages = []
        self.canvas.clear()
        self.canvas.enabled = False
        self.chat_input = ""
        self.status_message = "Connected!"
    
    def handle_events(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.running = False
            
            if self.state == STATE_LANDING:
                self.username_input.handle_event(event)
                self.room_code_input.handle_event(event)
                
                if self.btn_play_now.handle_event(event) and self.connected:
                    self.register_and_play_now()
                elif self.btn_create_room.handle_event(event) and self.connected:
                    self.register_and_create_room()
                elif self.btn_join_room.handle_event(event) and self.connected:
                    self.register_and_join_room()
            
            elif self.state in [STATE_WAITING, STATE_PLAYING]:
                if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                    self.canvas.handle_mouse_down(event.pos)
                    
                    if self.is_drawer and self.btn_clear.handle_event(event):
                        self.clear_canvas_action()
                    
                    # Color palette clicks
                    if self.is_drawer:
                        for i, rect in enumerate(self.color_buttons):
                            if rect.collidepoint(event.pos):
                                self.canvas.set_color(i)
                
                elif event.type == pygame.MOUSEBUTTONUP and event.button == 1:
                    self.canvas.handle_mouse_up(event.pos)
                
                elif event.type == pygame.MOUSEMOTION:
                    self.canvas.handle_mouse_move(event.pos, self.network)
                    if self.is_drawer:
                        self.btn_clear.handle_event(event)
                
                elif event.type == pygame.KEYDOWN:
                    if self.state == STATE_WAITING or (self.state == STATE_PLAYING and not self.is_drawer):
                        if event.key == pygame.K_RETURN:
                            self.send_chat()
                        elif event.key == pygame.K_BACKSPACE:
                            self.chat_input = self.chat_input[:-1]
                        elif event.unicode.isprintable() and len(self.chat_input) < 60:
                            self.chat_input += event.unicode
            
            elif self.state == STATE_ENDED:
                if self.btn_return_home.handle_event(event):
                    self.return_to_home()
    
    def render(self):
        self.screen.fill(BG_COLOR)
        
        if self.state == STATE_LANDING:
            self.render_landing()
        elif self.state in [STATE_WAITING, STATE_PLAYING]:
            self.render_game()
        elif self.state == STATE_ENDED:
            self.render_game_end()
        
        pygame.display.flip()
    
    def render_landing(self):
        # Title
        title = self.font_large.render("Scribble Game", True, BLACK)
        title_rect = title.get_rect(center=(WINDOW_WIDTH // 2, 100))
        self.screen.blit(title, title_rect)
        
        # Tagline
        tagline = self.font_small.render("Draw, Guess, Have Fun!", True, DARK_GRAY)
        tagline_rect = tagline.get_rect(center=(WINDOW_WIDTH // 2, 150))
        self.screen.blit(tagline, tagline_rect)
        
        # Status
        color = GREEN if self.connected else RED
        status = self.font_medium.render(self.status_message, True, color)
        status_rect = status.get_rect(center=(WINDOW_WIDTH // 2, 200))
        self.screen.blit(status, status_rect)
        
        # Inputs & buttons
        self.username_input.draw(self.screen, self.font_small)
        
        if self.connected:
            self.btn_play_now.draw(self.screen, self.font_medium)
            self.btn_create_room.draw(self.screen, self.font_medium)
            self.room_code_input.draw(self.screen, self.font_small)
            self.btn_join_room.draw(self.screen, self.font_small)
    
    def render_game(self):
        # Left sidebar - Players
        self.render_players()
        
        # Top info bar
        y = 20
        
        # Room code
        if self.room_code:
            room_text = self.font_small.render(f"Room: {self.room_code}", True, BLACK)
            self.screen.blit(room_text, (CANVAS_X, y))
        
        # Round
        if self.total_rounds > 0:
            round_text = self.font_small.render(f"Round {self.round_number}/{self.total_rounds}", True, BLACK)
            self.screen.blit(round_text, (CANVAS_X + 150, y))
        
        # Timer
        if self.timer > 0:
            color = RED if self.timer < 10 else BLACK
            timer_text = self.font_medium.render(f"{self.timer}s", True, color)
            self.screen.blit(timer_text, (CANVAS_X + CANVAS_WIDTH - 60, y))
        
        # Word (centered)
        y = 60
        if self.word_to_draw:
            word = self.font_large.render(self.word_to_draw, True, BLUE)
        else:
            word = self.font_large.render(self.word_mask, True, BLACK)
        
        word_rect = word.get_rect(center=(CANVAS_X + CANVAS_WIDTH // 2, y))
        self.screen.blit(word, word_rect)
        
        # Countdown
        if self.countdown > 0:
            countdown = self.font_large.render(f"Starting in {self.countdown}...", True, GREEN)
            countdown_rect = countdown.get_rect(center=(CANVAS_X + CANVAS_WIDTH // 2, y))
            self.screen.blit(countdown, countdown_rect)
        
        # Drawing tools (if drawer)
        if self.is_drawer:
            self.render_drawing_tools()
        
        # Canvas
        self.canvas.render(self.screen)
        
        # Right sidebar - Chat
        self.render_chat()
    
    def render_players(self):
        x = 10
        y = 80
        w = SIDEBAR_WIDTH - 20
        
        # Title
        title = self.font_medium.render("Players", True, BLACK)
        self.screen.blit(title, (x, 20))
        
        # Players list
        for player in self.players[:10]:
            username = player.get('username', 'Player')
            score = player.get('score', 0)
            player_id = player.get('player_id', 0)
            is_me = player_id == self.player_id
            online = player.get('online', True)
            
            # Background
            bg_color = (200, 220, 255) if is_me else WHITE
            pygame.draw.rect(self.screen, bg_color, (x, y, w, 40))
            pygame.draw.rect(self.screen, GRAY, (x, y, w, 40), 1)
            
            # Avatar circle
            colors = [(255, 173, 173), (255, 214, 165), (202, 255, 191), 
                     (155, 246, 255), (189, 178, 255), (255, 204, 229)]
            avatar_color = colors[player_id % len(colors)]
            pygame.draw.circle(self.screen, avatar_color, (x + 20, y + 20), 15)
            
            # Initial
            initial = username[0].upper() if username else 'P'
            initial_surf = self.font_small.render(initial, True, BLACK)
            initial_rect = initial_surf.get_rect(center=(x + 20, y + 20))
            self.screen.blit(initial_surf, initial_rect)
            
            # Online dot
            dot_color = GREEN if online else RED
            pygame.draw.circle(self.screen, dot_color, (x + 30, y + 28), 4)
            
            # Name & score
            name_surf = self.font_small.render(username[:12], True, BLACK)
            self.screen.blit(name_surf, (x + 40, y + 8))
            
            score_surf = self.font_small.render(f"{score} pts", True, DARK_GRAY)
            self.screen.blit(score_surf, (x + 40, y + 24))
            
            y += 45
    
    def render_drawing_tools(self):
        y = CANVAS_Y - 45
        
        # Clear button
        self.btn_clear.draw(self.screen, self.font_small)
        
        # Brush size
        brush_text = self.font_small.render(f"Brush: {self.canvas.line_width}px", True, BLACK)
        self.screen.blit(brush_text, (CANVAS_X + 430, y + 10))
        
        # Color palette
        for i, color in enumerate(COLORS[:10]):
            rect = self.color_buttons[i]
            pygame.draw.rect(self.screen, color, rect)
            
            if i == self.canvas.color_index:
                pygame.draw.rect(self.screen, YELLOW, rect, 3)
            else:
                pygame.draw.rect(self.screen, GRAY, rect, 1)
    
    def render_chat(self):
        x = CANVAS_X + CANVAS_WIDTH + 30
        y = 80
        w = WINDOW_WIDTH - x - 10
        h = CANVAS_HEIGHT + 40
        
        # Background
        pygame.draw.rect(self.screen, WHITE, (x, y, w, h))
        pygame.draw.rect(self.screen, GRAY, (x, y, w, h), 2)
        
        # Title
        title = self.font_medium.render("Chat", True, BLACK)
        self.screen.blit(title, (x + 10, 20))
        
        # Messages - THIS WAS THE BUG!
        msg_y = y + 10
        for msg in self.chat_messages[-25:]:
            if msg_y > y + h - 60:
                break
            
            # Truncate long messages
            display_msg = msg[:35] if len(msg) > 35 else msg
            
            if msg.startswith('*'):
                text = self.font_small.render(display_msg, True, DARK_GRAY)
            else:
                text = self.font_small.render(display_msg, True, BLACK)
            
            self.screen.blit(text, (x + 10, msg_y))
            msg_y += 20
        
        # Input box
        input_y = y + h - 45
        pygame.draw.rect(self.screen, LIGHT_GRAY, (x + 5, input_y, w - 10, 35))
        pygame.draw.rect(self.screen, GRAY, (x + 5, input_y, w - 10, 35), 2)
        
        if self.state == STATE_WAITING or (self.state == STATE_PLAYING and not self.is_drawer):
            if self.chat_input:
                input_text = self.font_small.render(self.chat_input[:30], True, BLACK)
            else:
                input_text = self.font_small.render("Type to chat...", True, DARK_GRAY)
            
            self.screen.blit(input_text, (x + 10, input_y + 10))
        else:
            hint = self.font_small.render("(Drawing...)", True, DARK_GRAY)
            self.screen.blit(hint, (x + 10, input_y + 10))
    
    def render_game_end(self):
        # Semi-transparent overlay
        overlay = pygame.Surface((WINDOW_WIDTH, WINDOW_HEIGHT))
        overlay.set_alpha(220)
        overlay.fill(WHITE)
        self.screen.blit(overlay, (0, 0))
        
        # Title
        title = self.font_large.render("Game Ended!", True, BLACK)
        title_rect = title.get_rect(center=(WINDOW_WIDTH // 2, 150))
        self.screen.blit(title, title_rect)
        
        # Rankings
        y = 250
        for i, player in enumerate(self.players[:5]):
            username = player.get('username', 'Player')
            score = player.get('score', 0)
            
            text = f"#{i+1}  {username}  -  {score} pts"
            if i == 0:
                text = f"WINNER: {text}"
                color = (255, 215, 0)  # Gold
            else:
                color = BLACK
            
            rank_surf = self.font_medium.render(text, True, color)
            rank_rect = rank_surf.get_rect(center=(WINDOW_WIDTH // 2, y))
            self.screen.blit(rank_surf, rank_rect)
            y += 50
        
        # Return button
        self.btn_return_home.draw(self.screen, self.font_medium)
    
    def try_reconnect(self):
        """Attempt to reconnect to server with session token"""
        if not self.session_token or self.reconnecting:
            return
        
        print("[Game] Connection lost, attempting to reconnect...")
        self.reconnecting = True
        self.status_message = "Reconnecting..."
        
        # Try to reconnect
        if self.network.connect(self.server_host, self.server_tcp_port):
            # Send reconnect request with session token
            self.network.send_tcp(MSG_TYPE.RECONNECT_REQUEST, {
                'session_token': self.session_token
            })
            print(f"[Game] Sent reconnect request with token: {self.session_token}")
        else:
            print("[Game] Failed to reconnect to server")
            self.reconnecting = False
            self.connected = False
            self.status_message = "Connection lost. Returning to menu..."
            pygame.time.wait(2000)
            self.return_to_home()
    
    def check_connection(self):
        """Check if connection is alive and attempt reconnect if needed"""
        current_time = pygame.time.get_ticks()
        
        # If we're in-game and lost connection
        if self.state in [STATE_WAITING, STATE_PLAYING] and not self.connected:
            if not self.reconnecting and self.session_token:
                self.try_reconnect()
        
        # Update ping time
        if self.connected and current_time - self.last_ping_time > 30000:  # 30 seconds
            self.last_ping_time = current_time
    
    def run(self):
        while self.running:
            # Network messages are handled via C callbacks automatically
            # Just check connection health
            self.check_connection()
            self.handle_events()
            self.render()
            self.clock.tick(FPS)
        
        self.network.disconnect()
        pygame.quit()
        sys.exit()


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Scribble Game Client')
    parser.add_argument('--host', default='localhost', help='Server IP')
    parser.add_argument('--tcp-port', type=int, default=9090, help='TCP port')
    args = parser.parse_args()
    
    game = ScribbleGame(host=args.host, tcp_port=args.tcp_port)
    game.run()
