// ParthenonChain - Network Protocol Implementation

#include "protocol.h"
#include <cstring>

namespace parthenon {
namespace p2p {

bool NetAddr::IsIPv4() const {
    // Check if it's an IPv4-mapped IPv6 address
    // Format: ::ffff:a.b.c.d
    static const uint8_t ipv4_prefix[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF
    };
    return std::memcmp(ip, ipv4_prefix, 12) == 0;
}

bool NetAddr::IsRoutable() const {
    // Check if address is publicly routable
    // Exclude localhost, private networks, etc.
    
    if (IsIPv4()) {
        // IPv4 checks
        uint32_t addr = (ip[12] << 24) | (ip[13] << 16) | (ip[14] << 8) | ip[15];
        
        // 0.0.0.0/8
        if ((addr & 0xFF000000) == 0x00000000) return false;
        
        // 10.0.0.0/8
        if ((addr & 0xFF000000) == 0x0A000000) return false;
        
        // 127.0.0.0/8
        if ((addr & 0xFF000000) == 0x7F000000) return false;
        
        // 169.254.0.0/16
        if ((addr & 0xFFFF0000) == 0xA9FE0000) return false;
        
        // 172.16.0.0/12
        if ((addr & 0xFFF00000) == 0xAC100000) return false;
        
        // 192.168.0.0/16
        if ((addr & 0xFFFF0000) == 0xC0A80000) return false;
        
        // 224.0.0.0/4 (multicast)
        if ((addr & 0xF0000000) == 0xE0000000) return false;
        
        // 240.0.0.0/4 (reserved)
        if ((addr & 0xF0000000) == 0xF0000000) return false;
        
        return true;
    }
    
    // IPv6 checks
    // ::1 (localhost)
    static const uint8_t ipv6_localhost[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    if (std::memcmp(ip, ipv6_localhost, 16) == 0) return false;
    
    // fc00::/7 (unique local)
    if ((ip[0] & 0xFE) == 0xFC) return false;
    
    // fe80::/10 (link local)
    if ((ip[0] == 0xFE) && ((ip[1] & 0xC0) == 0x80)) return false;
    
    return true;
}

} // namespace p2p
} // namespace parthenon
