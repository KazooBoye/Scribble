"""
Message protocol constants
Must match server/include/protocol.h
"""

class MSG_TYPE:
    # Client → Server (must match server/protocol.h)
    PING = 0
    PONG = 1
    REGISTER = 2
    REGISTER_ACK = 3
    JOIN_ROOM = 4
    CREATE_ROOM = 5
    ROOM_CREATED = 6
    ROOM_JOINED = 7
    ROOM_FULL = 8
    ROOM_NOT_FOUND = 9
    GAME_START = 10
    YOUR_TURN = 11
    WORD_TO_DRAW = 12
    ROUND_START = 13
    CHAT = 14
    CHAT_BROADCAST = 15
    GUESS_CORRECT = 16
    GUESS_WRONG = 17
    TIMER_UPDATE = 18
    COUNTDOWN_UPDATE = 19
    ROUND_END = 20
    GAME_END = 21
    PLAYER_JOIN = 22
    PLAYER_LEAVE = 23
    SCORE_UPDATE = 24
    RECONNECT_REQUEST = 25
    RECONNECT_SUCCESS = 26
    RECONNECT_FAIL = 27
    ERROR = 28
    DISCONNECT = 29
    
    # Host control messages (30+)
    HOST_START_GAME = 30
    HOST_KICK_PLAYER = 31
    
    # UDP Messages
    UDP_STROKE = 100
    UDP_CLEAR_CANVAS = 101
    UDP_UNDO = 102


# Color palette
COLORS = [
    (0, 0, 0),        # Black
    (255, 255, 255),  # White
    (255, 0, 0),      # Red
    (0, 255, 0),      # Green
    (0, 0, 255),      # Blue
    (255, 255, 0),    # Yellow
    (255, 165, 0),    # Orange
    (128, 0, 128),    # Purple
    (255, 192, 203),  # Pink
    (139, 69, 19),    # Brown
]


def get_color(index: int) -> tuple:
    """Get RGB color by index"""
    if 0 <= index < len(COLORS):
        return COLORS[index]
    return (0, 0, 0)
