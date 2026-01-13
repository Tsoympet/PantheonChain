// ParthenonChain - Peer Address Database Implementation

#include "peer_database.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace parthenon {
namespace p2p {

PeerDatabase::PeerDatabase() : is_open_(false) {}

PeerDatabase::~PeerDatabase() {
    Close();
}

bool PeerDatabase::Open(const std::string& db_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    db_path_ = db_path;
    
    // Load existing peers from file
    std::ifstream file(db_path_, std::ios::binary);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            PeerInfo info;
            std::string key;
            
            if (iss >> key >> info.address >> info.port >> info.services >>
                info.last_seen >> info.first_seen >> info.connection_attempts >>
                info.successful_connections >> info.failed_connections >>
                info.is_banned >> info.ban_until >> info.score >>
                info.blocks_received >> info.txs_received >>
                info.invalid_messages >> info.protocol_violations) {
                peers_[key] = info;
            }
        }
        file.close();
    }
    
    is_open_ = true;
    return true;
}

void PeerDatabase::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_open_) return;
    
    // Save all peers to file
    std::ofstream file(db_path_, std::ios::binary | std::ios::trunc);
    if (file.is_open()) {
        file << "# PantheonChain Peer Database\n";
        file << "# Format: key address port services last_seen first_seen attempts success fails banned ban_until score blocks txs invalid violations\n";
        
        for (const auto& pair : peers_) {
            const auto& key = pair.first;
            const auto& info = pair.second;
            
            file << key << " "
                 << info.address << " "
                 << info.port << " "
                 << info.services << " "
                 << info.last_seen << " "
                 << info.first_seen << " "
                 << info.connection_attempts << " "
                 << info.successful_connections << " "
                 << info.failed_connections << " "
                 << info.is_banned << " "
                 << info.ban_until << " "
                 << info.score << " "
                 << info.blocks_received << " "
                 << info.txs_received << " "
                 << info.invalid_messages << " "
                 << info.protocol_violations << "\n";
        }
        file.close();
    }
    
    is_open_ = false;
}

std::string PeerDatabase::MakeKey(const std::string& address, uint16_t port) {
    return address + ":" + std::to_string(port);
}

void PeerDatabase::AddPeer(const std::string& address, uint16_t port, uint64_t services) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = MakeKey(address, port);
    if (peers_.find(key) == peers_.end()) {
        PeerInfo info;
        info.address = address;
        info.port = port;
        info.services = services;
        info.first_seen = std::time(nullptr);
        info.last_seen = info.first_seen;
        info.score = 50.0; // Start with neutral score
        peers_[key] = info;
    }
}

void PeerDatabase::UpdatePeer(const std::string& address, uint16_t port, const PeerInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeKey(address, port);
    peers_[key] = info;
}

bool PeerDatabase::GetPeer(const std::string& address, uint16_t port, PeerInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeKey(address, port);
    auto it = peers_.find(key);
    if (it != peers_.end()) {
        info = it->second;
        return true;
    }
    return false;
}

std::vector<PeerInfo> PeerDatabase::GetPeers(size_t max_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<PeerInfo> result;
    result.reserve(std::min(max_count, peers_.size()));
    
    for (const auto& pair : peers_) {
        if (result.size() >= max_count) break;
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<PeerInfo> PeerDatabase::GetGoodPeers(size_t max_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<PeerInfo> all_peers;
    time_t now = std::time(nullptr);
    
    for (const auto& pair : peers_) {
        const auto& info = pair.second;
        // Filter out banned peers and peers with bad scores
        if (!info.is_banned && info.score > 25.0 &&
            (info.ban_until == 0 || info.ban_until < now)) {
            all_peers.push_back(info);
        }
    }
    
    // Sort by score (highest first)
    std::sort(all_peers.begin(), all_peers.end(),
              [](const PeerInfo& a, const PeerInfo& b) {
                  return a.score > b.score;
              });
    
    // Return top peers
    if (all_peers.size() > max_count) {
        all_peers.resize(max_count);
    }
    
    return all_peers;
}

void PeerDatabase::RecordConnectionAttempt(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeKey(address, port);
    auto& info = peers_[key];
    info.connection_attempts++;
}

void PeerDatabase::RecordSuccessfulConnection(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeKey(address, port);
    auto& info = peers_[key];
    info.successful_connections++;
    info.last_seen = std::time(nullptr);
    // Reward successful connections
    info.score += 5.0;
    if (info.score > 100.0) info.score = 100.0;
}

void PeerDatabase::RecordFailedConnection(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeKey(address, port);
    auto& info = peers_[key];
    info.failed_connections++;
    // Penalize failed connections
    info.score -= 2.0;
    if (info.score < 0.0) info.score = 0.0;
}

void PeerDatabase::UpdateLastSeen(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeKey(address, port);
    auto& info = peers_[key];
    info.last_seen = std::time(nullptr);
}

void PeerDatabase::BanPeer(const std::string& address, uint16_t port, time_t duration_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeKey(address, port);
    auto& info = peers_[key];
    info.is_banned = true;
    info.ban_until = std::time(nullptr) + duration_seconds;
    info.score = 0.0; // Reset score to minimum
}

void PeerDatabase::UnbanPeer(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeKey(address, port);
    auto& info = peers_[key];
    info.is_banned = false;
    info.ban_until = 0;
    info.score = 25.0; // Reset to low but non-zero score
}

bool PeerDatabase::IsBanned(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeKey(address, port);
    auto it = peers_.find(key);
    if (it != peers_.end()) {
        time_t now = std::time(nullptr);
        // Check if ban has expired
        if (it->second.ban_until > 0 && it->second.ban_until < now) {
            it->second.is_banned = false;
            it->second.ban_until = 0;
            return false;
        }
        return it->second.is_banned;
    }
    return false;
}

void PeerDatabase::UpdateScore(const std::string& address, uint16_t port, double delta) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeKey(address, port);
    auto& info = peers_[key];
    info.score += delta;
    if (info.score < 0.0) info.score = 0.0;
    if (info.score > 100.0) info.score = 100.0;
}

void PeerDatabase::RecordBlockReceived(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeKey(address, port);
    auto& info = peers_[key];
    info.blocks_received++;
    info.score += 1.0; // Reward block sharing
    if (info.score > 100.0) info.score = 100.0;
}

void PeerDatabase::RecordTxReceived(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeKey(address, port);
    auto& info = peers_[key];
    info.txs_received++;
    info.score += 0.1; // Small reward for tx sharing
    if (info.score > 100.0) info.score = 100.0;
}

void PeerDatabase::RecordInvalidMessage(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeKey(address, port);
    auto& info = peers_[key];
    info.invalid_messages++;
    info.score -= 5.0; // Penalty for invalid messages
    if (info.score < 0.0) info.score = 0.0;
}

void PeerDatabase::RecordProtocolViolation(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeKey(address, port);
    auto& info = peers_[key];
    info.protocol_violations++;
    info.score -= 10.0; // Heavy penalty for protocol violations
    if (info.score < 0.0) info.score = 0.0;
    
    // Auto-ban peers with too many violations
    if (info.protocol_violations >= 5) {
        info.is_banned = true;
        info.ban_until = std::time(nullptr) + 86400; // Ban for 24 hours
    }
}

size_t PeerDatabase::GetPeerCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    return peers_.size();
}

size_t PeerDatabase::GetBannedCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    time_t now = std::time(nullptr);
    for (const auto& pair : peers_) {
        if (pair.second.is_banned && 
            (pair.second.ban_until == 0 || pair.second.ban_until >= now)) {
            count++;
        }
    }
    return count;
}

} // namespace p2p
} // namespace parthenon
