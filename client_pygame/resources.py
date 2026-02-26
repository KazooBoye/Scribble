"""
Resource manager for loading images and creating fallback graphics
"""
import pygame
import os

class ResourceManager:
    def __init__(self):
        self.res_dir = os.path.join(os.path.dirname(__file__), 'res')
        self.images = {}
        self.load_resources()
    
    def load_resources(self):
        """Load all image resources, create fallbacks if missing"""
        # Try to load images, create fallbacks if they don't exist
        self.images['avatar_default'] = self.load_or_create('avatar_default.png', (32, 32), self.create_avatar_icon)
        self.images['icon_online'] = self.load_or_create('icon_online.png', (10, 10), self.create_online_icon)
        self.images['icon_offline'] = self.load_or_create('icon_offline.png', (10, 10), self.create_offline_icon)
        self.images['icon_drawing'] = self.load_or_create('icon_drawing.png', (16, 16), self.create_drawing_icon)
        self.images['icon_clear'] = self.load_or_create('icon_clear.png', (24, 24), self.create_clear_icon)
        self.images['icon_send'] = self.load_or_create('icon_send.png', (20, 20), self.create_send_icon)
        self.images['icon_kick'] = self.load_or_create('icon_kick.png', (16, 16), self.create_kick_icon)
        
        # Optional: Load background and logo if available
        self.images['bg_wood'] = self.try_load('bg_wood.jpg')
        self.images['logo'] = self.try_load('logo.png')
    
    def load_or_create(self, filename, size, create_func):
        """Load image from file or create it programmatically"""
        filepath = os.path.join(self.res_dir, filename)
        
        if os.path.exists(filepath):
            try:
                return pygame.image.load(filepath).convert_alpha()
            except:
                print(f"[Resources] Failed to load {filename}, creating fallback")
        
        # Create fallback
        return create_func(size)
    
    def try_load(self, filename):
        """Try to load optional resource, return None if not found"""
        filepath = os.path.join(self.res_dir, filename)
        
        if os.path.exists(filepath):
            try:
                return pygame.image.load(filepath).convert_alpha()
            except:
                print(f"[Resources] Failed to load optional resource {filename}")
        
        return None
    
    def create_avatar_icon(self, size):
        """Create simple person silhouette"""
        surf = pygame.Surface(size, pygame.SRCALPHA)
        w, h = size
        # Head
        pygame.draw.circle(surf, (100, 100, 100), (w//2, h//3), w//4)
        # Body
        pygame.draw.circle(surf, (100, 100, 100), (w//2, h*2//3), w//3)
        return surf
    
    def create_online_icon(self, size):
        """Create green circle"""
        surf = pygame.Surface(size, pygame.SRCALPHA)
        pygame.draw.circle(surf, (76, 175, 80), (size[0]//2, size[1]//2), size[0]//2)
        return surf
    
    def create_offline_icon(self, size):
        """Create red circle"""
        surf = pygame.Surface(size, pygame.SRCALPHA)
        pygame.draw.circle(surf, (244, 67, 54), (size[0]//2, size[1]//2), size[0]//2)
        return surf
    
    def create_drawing_icon(self, size):
        """Create pencil icon"""
        surf = pygame.Surface(size, pygame.SRCALPHA)
        w, h = size
        # Simple pencil shape
        points = [(w*0.3, h*0.7), (w*0.7, h*0.3), (w*0.5, h*0.1), (w*0.1, h*0.5)]
        pygame.draw.polygon(surf, (255, 193, 7), points)
        return surf
    
    def create_send_icon(self, size):
        """Create send/paper plane icon"""
        surf = pygame.Surface(size, pygame.SRCALPHA)
        w, h = size
        # Simple arrow/plane shape
        points = [(w*0.1, h*0.5), (w*0.9, h*0.3), (w*0.9, h*0.7)]
        pygame.draw.polygon(surf, (70, 130, 220), points)
        return surf
    
    def create_kick_icon(self, size):
        """Create kick/remove icon (boot or X in circle)"""
        surf = pygame.Surface(size, pygame.SRCALPHA)
        w, h = size
        # Red circle with X
        pygame.draw.circle(surf, (244, 67, 54), (w//2, h//2), w//2)
        # White X inside
        line_width = max(2, w//8)
        pygame.draw.line(surf, (255, 255, 255), (w*0.3, h*0.3), (w*0.7, h*0.7), line_width)
        pygame.draw.line(surf, (255, 255, 255), (w*0.7, h*0.3), (w*0.3, h*0.7), line_width)
        return surf
    
    def create_clear_icon(self, size):
        """Create eraser/trash icon"""
        surf = pygame.Surface(size, pygame.SRCALPHA)
        w, h = size
        # Simple X
        pygame.draw.line(surf, (244, 67, 54), (w*0.2, h*0.2), (w*0.8, h*0.8), 3)
        pygame.draw.line(surf, (244, 67, 54), (w*0.8, h*0.2), (w*0.2, h*0.8), 3)
        return surf
    
    def get(self, name):
        """Get loaded resource"""
        return self.images.get(name)
