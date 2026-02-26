"""
Python ctypes wrapper for C networking library
This module loads the C shared library and provides Python interface
"""

import ctypes
import json
import os
from typing import Callable, Dict, Any
from threading import Lock

# Find the shared library
lib_path = os.path.join(os.path.dirname(__file__), '..', 'build', 'libscribble_client.so')
if not os.path.exists(lib_path):
    # Try dylib for macOS
    lib_path = os.path.join(os.path.dirname(__file__), '..', 'build', 'libscribble_client.dylib')

# Load C library
try:
    _network_lib = ctypes.CDLL(lib_path)
except OSError as e:
    print(f"Failed to load C library: {e}")
    print(f"Make sure to build the library first: make client")
    raise

# Define C function signatures
_network_lib.network_connect.argtypes = [ctypes.c_char_p, ctypes.c_int]
_network_lib.network_connect.restype = ctypes.c_int

_network_lib.network_send_tcp.argtypes = [ctypes.c_char_p]
_network_lib.network_send_tcp.restype = ctypes.c_int

_network_lib.network_is_connected.argtypes = []
_network_lib.network_is_connected.restype = ctypes.c_int

_network_lib.network_disconnect.argtypes = []
_network_lib.network_disconnect.restype = None

_network_lib.network_get_error.argtypes = []
_network_lib.network_get_error.restype = ctypes.c_char_p

# Define callback type
MESSAGE_CALLBACK_TYPE = ctypes.CFUNCTYPE(None, ctypes.c_char_p)
_network_lib.network_set_callback.argtypes = [MESSAGE_CALLBACK_TYPE]
_network_lib.network_set_callback.restype = None


class NetworkClient:
    """Python wrapper for C networking library"""
    
    def __init__(self):
        self.connected = False
        self.message_handlers = {}
        self.callback_lock = Lock()
        
        # Keep reference to prevent garbage collection
        self._c_callback = MESSAGE_CALLBACK_TYPE(self._message_callback)
        _network_lib.network_set_callback(self._c_callback)
    
    def _message_callback(self, json_bytes):
        """Called from C thread when message is received"""
        try:
            json_str = json_bytes.decode('utf-8')
            message = json.loads(json_str)
            
            msg_type = message.get('type')
            msg_data = message.get('data', {})
            
            # Thread-safe handler lookup
            with self.callback_lock:
                handler = self.message_handlers.get(msg_type)
            
            if handler:
                handler(msg_data)
                
        except Exception as e:
            print(f"[Network] Callback error: {e}")
    
    def connect(self, host: str = 'localhost', tcp_port: int = 9090) -> bool:
        """Connect to server"""
        host_bytes = host.encode('utf-8')
        result = _network_lib.network_connect(host_bytes, tcp_port)
        
        if result == 0:
            self.connected = True
            print(f"[Network] Connected to {host}:{tcp_port}")
            return True
        else:
            error = _network_lib.network_get_error().decode('utf-8')
            print(f"[Network] Connection failed: {error}")
            return False
    
    def send_tcp(self, msg_type: int, data: Dict[Any, Any]) -> bool:
        """Send message via TCP"""
        if not self.is_connected():
            return False
        
        message = {'type': msg_type, 'data': data}
        json_str = json.dumps(message)
        json_bytes = json_str.encode('utf-8')
        
        result = _network_lib.network_send_tcp(json_bytes)
        return result == 0
    
    def register_handler(self, msg_type: int, handler: Callable):
        """Register callback for message type"""
        with self.callback_lock:
            self.message_handlers[msg_type] = handler
    
    def is_connected(self) -> bool:
        """Check connection status"""
        return _network_lib.network_is_connected() == 1
    
    def disconnect(self):
        """Disconnect from server"""
        _network_lib.network_disconnect()
        self.connected = False
    
    def get_error(self) -> str:
        """Get last error message"""
        return _network_lib.network_get_error().decode('utf-8')
