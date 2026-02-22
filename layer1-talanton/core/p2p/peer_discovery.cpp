// ParthenonChain - Peer Discovery Implementation

#include "peer_discovery.h"

#include <algorithm>
#include <cstring>
#include <sstream>

// Platform-specific networking headers
#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#endif
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif
#include <cstring>
#include <sstream>

namespace parthenon {
namespace p2p {

// Default DNS seeds (should be customized for production)
const std::vector<std::string> PeerDiscovery::DEFAULT_DNS_SEEDS = {
    "seed.parthenon.network", "dnsseed.parthenon.io", "seed1.parthenon.network",
    "seed2.parthenon.network"};

// Default seed nodes (should be customized for production)
const std::vector<std::string> PeerDiscovery::DEFAULT_SEED_NODES = {
    "52.14.78.91:8333",     // Example seed node 1
    "35.162.213.114:8333",  // Example seed node 2
    "18.217.83.46:8333",    // Example seed node 3
    "13.52.234.101:8333"    // Example seed node 4
};

PeerDiscovery::PeerDiscovery(PeerDatabase& peer_db)
    : peer_db_(peer_db),
      dns_discovery_enabled_(true),
      peer_exchange_enabled_(true),
      peer_discovered_callback_(nullptr) {
#ifdef _WIN32
    // Initialize WinSock for Windows networking
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        // WSAStartup failed - networking operations will fail gracefully later
        // Store the error for diagnostic purposes but do not abort construction
        wsa_startup_failed_ = true;
    }
#endif
}

PeerDiscovery::~PeerDiscovery() {
#ifdef _WIN32
    // Cleanup WinSock on Windows
    WSACleanup();
#endif
}

bool PeerDiscovery::ParseAddressPort(const std::string& addr_str, std::string& address,
                                     uint16_t& port) {
    size_t colon_pos = addr_str.rfind(':');
    if (colon_pos == std::string::npos) {
        return false;
    }

    address = addr_str.substr(0, colon_pos);
    std::string port_str = addr_str.substr(colon_pos + 1);

    try {
        int port_int = std::stoi(port_str);
        if (port_int <= 0 || port_int > 65535) {
            return false;
        }
        port = static_cast<uint16_t>(port_int);
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<std::string> PeerDiscovery::QueryDNS(const std::string& hostname) {
    std::vector<std::string> addresses;

    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (status != 0) {
        // DNS query failed
        return addresses;
    }

    // Iterate through results
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in* ipv4 = reinterpret_cast<struct sockaddr_in*>(rp->ai_addr);
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, INET_ADDRSTRLEN);
            addresses.push_back(std::string(ip_str));
        }
    }

    freeaddrinfo(result);
    return addresses;
}

bool PeerDiscovery::ValidatePeerAddress(const std::string& address) {
    // Basic validation: reject obviously invalid addresses
    if (address.empty()) {
        return false;
    }

    // Reject localhost addresses
    if (address == "127.0.0.1" || address == "localhost" || address == "::1") {
        return false;
    }

    // Reject private IP ranges (for mainnet, allow for testnet/regtest)
    // 10.0.0.0/8
    if (address.substr(0, 3) == "10.") {
        return false;
    }

    // 172.16.0.0/12
    if (address.substr(0, 4) == "172.") {
        std::string second_octet = address.substr(4);
        size_t dot_pos = second_octet.find('.');
        if (dot_pos != std::string::npos) {
            try {
                int octet = std::stoi(second_octet.substr(0, dot_pos));
                if (octet >= 16 && octet <= 31) {
                    return false;
                }
            } catch (...) {
            }
        }
    }

    // 192.168.0.0/16
    if (address.substr(0, 8) == "192.168.") {
        return false;
    }

    return true;
}

void PeerDiscovery::NotifyPeerDiscovered(const std::string& address, uint16_t port) {
    if (peer_discovered_callback_) {
        peer_discovered_callback_(address, port);
    }
}

size_t PeerDiscovery::DiscoverFromDNS(const std::vector<std::string>& dns_seeds) {
    if (!dns_discovery_enabled_) {
        return 0;
    }

    size_t discovered_count = 0;

    for (const auto& seed : dns_seeds) {
        auto addresses = QueryDNS(seed);

        for (const auto& address : addresses) {
            if (ValidatePeerAddress(address)) {
                // Default port for mainnet
                uint16_t port = 8333;

                // Add to peer database
                peer_db_.AddPeer(address, port);
                discovered_count++;

                NotifyPeerDiscovered(address, port);
            }
        }
    }

    return discovered_count;
}

size_t PeerDiscovery::AddSeedNodes(const std::vector<std::string>& seed_nodes) {
    size_t added_count = 0;

    for (const auto& node : seed_nodes) {
        std::string address;
        uint16_t port;

        if (ParseAddressPort(node, address, port) && ValidatePeerAddress(address)) {
            peer_db_.AddPeer(address, port);
            added_count++;

            NotifyPeerDiscovered(address, port);
        }
    }

    return added_count;
}

size_t PeerDiscovery::DiscoverFromPeerExchange(
    const std::vector<std::pair<std::string, uint16_t>>& peers) {
    if (!peer_exchange_enabled_) {
        return 0;
    }

    size_t added_count = 0;

    for (const auto& peer : peers) {
        const auto& address = peer.first;
        uint16_t port = peer.second;

        if (ValidatePeerAddress(address)) {
            peer_db_.AddPeer(address, port);
            added_count++;

            NotifyPeerDiscovered(address, port);
        }
    }

    return added_count;
}

std::vector<std::pair<std::string, uint16_t>> PeerDiscovery::GetInitialPeers(size_t count) {
    std::vector<std::pair<std::string, uint16_t>> result;

    // First, try to get good peers from database
    auto good_peers = peer_db_.GetGoodPeers(count);

    for (const auto& peer : good_peers) {
        result.push_back({peer.address, peer.port});
    }

    // If we don't have enough, add seed nodes
    if (result.size() < count) {
        AddSeedNodes(DEFAULT_SEED_NODES);

        // Get peers again
        good_peers = peer_db_.GetGoodPeers(count);
        result.clear();

        for (const auto& peer : good_peers) {
            result.push_back({peer.address, peer.port});
            if (result.size() >= count)
                break;
        }
    }

    // If still not enough, try DNS discovery
    if (result.size() < count && dns_discovery_enabled_) {
        DiscoverFromDNS(DEFAULT_DNS_SEEDS);

        // Get peers again
        good_peers = peer_db_.GetGoodPeers(count);
        result.clear();

        for (const auto& peer : good_peers) {
            result.push_back({peer.address, peer.port});
            if (result.size() >= count)
                break;
        }
    }

    return result;
}

std::vector<std::pair<std::string, uint16_t>> PeerDiscovery::GetDiversePeers(size_t count) {
    std::vector<std::pair<std::string, uint16_t>> result;

    // Get geographically diverse peers
    auto diverse_peers = peer_db_.GetGeographicallyDiversePeers(count);

    for (const auto& peer : diverse_peers) {
        result.push_back({peer.address, peer.port});
        if (result.size() >= count)
            break;
    }

    // If we don't have enough diverse peers, fall back to regular good peers
    if (result.size() < count) {
        auto good_peers = peer_db_.GetGoodPeers(count);
        result.clear();

        for (const auto& peer : good_peers) {
            result.push_back({peer.address, peer.port});
            if (result.size() >= count)
                break;
        }
    }

    return result;
}

size_t PeerDiscovery::PeriodicDiscovery() {
    size_t discovered = 0;

    // Get current peer count
    size_t current_peers = peer_db_.GetPeerCount();

    // If we have very few peers, do aggressive discovery
    if (current_peers < 10) {
        // Add seed nodes
        discovered += AddSeedNodes(DEFAULT_SEED_NODES);

        // Try DNS discovery
        if (dns_discovery_enabled_) {
            discovered += DiscoverFromDNS(DEFAULT_DNS_SEEDS);
        }
    }
    // If we have moderate number of peers, do light discovery
    else if (current_peers < 100) {
        // Occasionally try DNS to find new peers
        if (dns_discovery_enabled_) {
            discovered += DiscoverFromDNS(DEFAULT_DNS_SEEDS);
        }
    }

    return discovered;
}

}  // namespace p2p
}  // namespace parthenon
