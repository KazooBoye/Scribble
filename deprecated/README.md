# Deprecated Code

This directory contains code that has been deprecated and removed from the active codebase.

## UDP Implementation (deprecated 2024)

### Files:
- `udp/udp_server.c` - UDP server socket handling
- `udp/udp_server.h` - UDP server interface
- `udp/udp_broadcast.c` - Binary UDP packet serialization
- `udp/udp_broadcast.h` - UDP broadcast interface

### Reason for Deprecation:
The UDP implementation was originally created to handle drawing stroke broadcasting for better performance. However, it proved to be overly complex due to:

1. **Hybrid architecture complexity**: Required maintaining both UDP and TCP sockets, with UDP packets needing to be converted back to TCP for broadcasting
2. **Player lookup issues**: Server needed to match UDP packets to TCP connections via IP address
3. **Binary protocol overhead**: Binary packing/unpacking added complexity without significant benefits
4. **Maintenance burden**: Two separate protocols for essentially the same data

### Current Solution:
All drawing strokes and canvas operations are now handled via TCP with JSON messages (types 100-102). This simplified approach:
- Uses single TCP connection for all communication
- Maintains JSON consistency throughout the protocol
- Reduces code complexity
- Performs adequately for the use case

### Message Types:
The following message type numbers (100-102) are still in use but now sent over TCP:
- `100` (UDP_STROKE) - Drawing stroke data
- `101` (UDP_CLEAR_CANVAS) - Clear canvas command
- `102` (UDP_UNDO) - Undo last stroke

These retain their "UDP" prefix in the codebase for historical reasons but are transmitted via TCP.

### Date Deprecated:
December 2024
