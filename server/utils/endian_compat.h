#ifndef ENDIAN_COMPAT_H
#define ENDIAN_COMPAT_H

#include <stdint.h>

// Check if we're on a system with endian.h
#ifdef __linux__
    #include <endian.h>
    
    // Some Linux systems don't have htobe64/be64toh in endian.h
    #ifndef htobe64
        #include <arpa/inet.h>
        static inline uint64_t htobe64(uint64_t host_64bits) {
            return ((uint64_t)htonl((uint32_t)host_64bits) << 32) | htonl((uint32_t)(host_64bits >> 32));
        }
    #endif
    
    #ifndef be64toh
        #include <arpa/inet.h>
        static inline uint64_t be64toh(uint64_t big_endian_64bits) {
            return ((uint64_t)ntohl((uint32_t)big_endian_64bits) << 32) | ntohl((uint32_t)(big_endian_64bits >> 32));
        }
    #endif
    
#elif defined(__APPLE__)
    #include <libkern/OSByteOrder.h>
    #define htobe64(x) OSSwapHostToBigInt64(x)
    #define be64toh(x) OSSwapBigToHostInt64(x)
    
#elif defined(_WIN32)
    #include <winsock2.h>
    #define htobe64(x) htonll(x)
    #define be64toh(x) ntohll(x)
    
    // Define htonll and ntohll if not available
    #ifndef htonll
    static inline uint64_t htonll(uint64_t x) {
        return ((uint64_t)htonl((uint32_t)x) << 32) | htonl((uint32_t)(x >> 32));
    }
    #endif
    
    #ifndef ntohll
    static inline uint64_t ntohll(uint64_t x) {
        return ((uint64_t)ntohl((uint32_t)x) << 32) | ntohl((uint32_t)(x >> 32));
    }
    #endif
    
#else
    // Fallback implementation for other systems
    #include <arpa/inet.h>
    
    static inline uint64_t htobe64(uint64_t host_64bits) {
        return ((uint64_t)htonl((uint32_t)host_64bits) << 32) | htonl((uint32_t)(host_64bits >> 32));
    }
    
    static inline uint64_t be64toh(uint64_t big_endian_64bits) {
        return ((uint64_t)ntohl((uint32_t)big_endian_64bits) << 32) | ntohl((uint32_t)(big_endian_64bits >> 32));
    }
#endif

#endif // ENDIAN_COMPAT_H
