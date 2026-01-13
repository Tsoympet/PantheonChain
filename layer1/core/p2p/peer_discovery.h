// ParthenonChain - Peer Discovery
// DNS seeding and peer discovery mechanisms

#pragma once

#include "peer_database.h"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace parthenon {
namespace p2p {

/**
 * Peer discovery mechanism
 * Supports DNS seeding, hardcoded seed nodes, and peer exchange
 */
class PeerDiscovery {
public:
    PeerDiscovery(PeerDatabase& peer_db);
    ~PeerDiscovery();
    
    /**
     * DNS Seeding - Query DNS seeds for initial peers
     * @param dns_seeds List of DNS seed domains
     * @return Number of peers discovered
     */
    size_t DiscoverFromDNS(const std::vector<std::string>& dns_seeds);
    
    /**
     * Add hardcoded seed nodes
     * @param seed_nodes List of "address:port" strings
     * @return Number of seeds added
     */
    size_t AddSeedNodes(const std::vector<std::string>& seed_nodes);
    
    /**
     * Discover peers from addr messages (peer exchange)
     * @param peers List of peer addresses from network messages
     * @return Number of new peers added
     */
    size_t DiscoverFromPeerExchange(const std::vector<std::pair<std::string, uint16_t>>& peers);
    
    /**
     * Get initial peers to connect to
     * @param count Number of peers to return
     * @return List of peer addresses
     */
    std::vector<std::pair<std::string, uint16_t>> GetInitialPeers(size_t count = 8);
    
    /**
     * Get diverse peers for new connections
     * @param count Number of peers to return
     * @return List of geographically diverse peers
     */
    std::vector<std::pair<std::string, uint16_t>> GetDiversePeers(size_t count = 8);
    
    /**
     * Enable/disable DNS discovery
     */
    void SetDNSDiscoveryEnabled(bool enabled) { dns_discovery_enabled_ = enabled; }
    
    /**
     * Enable/disable peer exchange
     */
    void SetPeerExchangeEnabled(bool enabled) { peer_exchange_enabled_ = enabled; }
    
    /**
     * Set callback for when new peers are discovered
     */
    using PeerDiscoveredCallback = std::function<void(const std::string&, uint16_t)>;
    void SetPeerDiscoveredCallback(PeerDiscoveredCallback callback) {
        peer_discovered_callback_ = callback;
    }
    
    /**
     * Perform periodic peer discovery (should be called regularly)
     * @return Number of new peers discovered
     */
    size_t PeriodicDiscovery();
    
private:
    /**
     * Parse address:port string
     */
    bool ParseAddressPort(const std::string& addr_str, std::string& address, uint16_t& port);
    
    /**
     * Query DNS for IPv4 addresses
     */
    std::vector<std::string> QueryDNS(const std::string& hostname);
    
    /**
     * Validate peer address (reject local/private addresses on mainnet)
     */
    bool ValidatePeerAddress(const std::string& address);
    
    /**
     * Notify about discovered peer
     */
    void NotifyPeerDiscovered(const std::string& address, uint16_t port);
    
    PeerDatabase& peer_db_;
    bool dns_discovery_enabled_;
    bool peer_exchange_enabled_;
    PeerDiscoveredCallback peer_discovered_callback_;
    
    // Default DNS seeds for mainnet
    static const std::vector<std::string> DEFAULT_DNS_SEEDS;
    
    // Default hardcoded seed nodes (mainnet)
    static const std::vector<std::string> DEFAULT_SEED_NODES;
};

} // namespace p2p
} // namespace parthenon
