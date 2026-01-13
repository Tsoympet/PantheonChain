// ParthenonChain - Peer Address Database
// Persistent storage for peer addresses and connection history

#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <ctime>
#include <mutex>

namespace parthenon {
namespace p2p {

/**
 * Information about a known peer
 */
struct PeerInfo {
    std::string address;
    uint16_t port;
    uint64_t services;
    time_t last_seen;
    time_t first_seen;
    uint32_t connection_attempts;
    uint32_t successful_connections;
    uint32_t failed_connections;
    bool is_banned;
    time_t ban_until;
    
    // Peer scoring metrics
    double score;
    uint32_t blocks_received;
    uint32_t txs_received;
    uint32_t invalid_messages;
    uint32_t protocol_violations;
    
    // Geographic diversity tracking
    std::string country_code;  // ISO 3166-1 alpha-2 (e.g., "US", "CN", "DE")
    std::string asn;           // Autonomous System Number
    double latitude;
    double longitude;
    std::string isp;
    
    PeerInfo() : port(0), services(0), last_seen(0), first_seen(0),
                 connection_attempts(0), successful_connections(0),
                 failed_connections(0), is_banned(false), ban_until(0),
                 score(0.0), blocks_received(0), txs_received(0),
                 invalid_messages(0), protocol_violations(0),
                 latitude(0.0), longitude(0.0) {}
};

/**
 * Persistent database for peer addresses
 */
class PeerDatabase {
public:
    PeerDatabase();
    ~PeerDatabase();
    
    // Database operations
    bool Open(const std::string& db_path);
    void Close();
    
    // Peer management
    void AddPeer(const std::string& address, uint16_t port, uint64_t services = 0);
    void UpdatePeer(const std::string& address, uint16_t port, const PeerInfo& info);
    bool GetPeer(const std::string& address, uint16_t port, PeerInfo& info);
    std::vector<PeerInfo> GetPeers(size_t max_count = 1000);
    std::vector<PeerInfo> GetGoodPeers(size_t max_count = 100);
    
    // Connection tracking
    void RecordConnectionAttempt(const std::string& address, uint16_t port);
    void RecordSuccessfulConnection(const std::string& address, uint16_t port);
    void RecordFailedConnection(const std::string& address, uint16_t port);
    void UpdateLastSeen(const std::string& address, uint16_t port);
    
    // Banning
    void BanPeer(const std::string& address, uint16_t port, time_t duration_seconds = 86400);
    void UnbanPeer(const std::string& address, uint16_t port);
    bool IsBanned(const std::string& address, uint16_t port);
    
    // Scoring
    void UpdateScore(const std::string& address, uint16_t port, double delta);
    void RecordBlockReceived(const std::string& address, uint16_t port);
    void RecordTxReceived(const std::string& address, uint16_t port);
    void RecordInvalidMessage(const std::string& address, uint16_t port);
    void RecordProtocolViolation(const std::string& address, uint16_t port);
    
    // Statistics
    size_t GetPeerCount();
    size_t GetBannedCount();
    
    // Geographic diversity
    void SetPeerGeolocation(const std::string& address, uint16_t port,
                            const std::string& country_code,
                            const std::string& asn,
                            double latitude, double longitude,
                            const std::string& isp = "");
    std::map<std::string, size_t> GetCountryDistribution();
    std::map<std::string, size_t> GetASNDistribution();
    std::vector<PeerInfo> GetGeographicallyDiversePeers(size_t max_count = 100);
    
private:
    std::string MakeKey(const std::string& address, uint16_t port);
    void SavePeer(const std::string& key, const PeerInfo& info);
    bool LoadPeer(const std::string& key, PeerInfo& info);
    
    std::string db_path_;
    std::map<std::string, PeerInfo> peers_;
    mutable std::mutex mutex_;
    bool is_open_;
};

} // namespace p2p
} // namespace parthenon
