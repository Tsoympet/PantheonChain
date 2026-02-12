// ParthenonChain - P2P Network Manager
// TCP socket-based networking for blockchain P2P

#pragma once

#include "primitives/block.h"
#include "primitives/transaction.h"

#include "message.h"
#include "protocol.h"

// Platform-specific networking headers
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <thread>
#include <vector>
#include <cstddef>

namespace parthenon {
namespace p2p {

/**
 * Peer connection state
 */
enum class PeerState { CONNECTING, HANDSHAKE, CONNECTED, DISCONNECTED, BANNED };

/**
 * Represents a single peer connection
 */
class PeerConnection {
  public:
    PeerConnection(int socket_fd, const std::string& address, uint16_t port,
                   uint32_t network_magic);
    ~PeerConnection();

    // Connection management
    bool Connect();
    void Disconnect();
    bool IsConnected() const { return state_ == PeerState::CONNECTED; }

    // Message sending
    bool SendVersion(const VersionMessage& msg);
    bool SendVerack();
    bool SendPing(uint64_t nonce);
    bool SendPong(uint64_t nonce);
    bool SendGetHeaders(const GetHeadersMessage& msg);
    bool SendGetData(const GetDataMessage& msg);
    bool SendInv(const InvMessage& msg);
    bool SendBlock(const primitives::Block& block);
    bool SendTx(const primitives::Transaction& tx);
    bool SendAddr(const AddrMessage& msg);

    // Message receiving
    bool ReceiveMessage();

    // Getters
    std::string GetAddress() const { return address_; }
    uint16_t GetPort() const { return port_; }
    PeerState GetState() const { return state_; }
    uint32_t GetVersion() const { return version_; }
    uint32_t GetHeight() const { return height_; }
    uint64_t GetServices() const { return services_; }

    // Callbacks
    void SetOnVersion(std::function<void(const VersionMessage&)> callback) {
        on_version_ = callback;
    }
    void SetOnVerack(std::function<void()> callback) { on_verack_ = callback; }
    void SetOnPing(std::function<void(uint64_t)> callback) { on_ping_ = callback; }
    void SetOnPong(std::function<void(uint64_t)> callback) { on_pong_ = callback; }
    void SetOnInv(std::function<void(const InvMessage&)> callback) { on_inv_ = callback; }
    void SetOnGetData(std::function<void(const GetDataMessage&)> callback) {
        on_getdata_ = callback;
    }
    void SetOnBlock(std::function<void(const primitives::Block&)> callback) {
        on_block_ = callback;
    }
    void SetOnTx(std::function<void(const primitives::Transaction&)> callback) {
        on_tx_ = callback;
    }
    void SetOnAddr(std::function<void(const AddrMessage&)> callback) { on_addr_ = callback; }
    void SetOnGetHeaders(std::function<void(const GetHeadersMessage&)> callback) {
        on_getheaders_ = callback;
    }
    void SetOnHeaders(std::function<void(const HeadersMessage&)> callback) {
        on_headers_ = callback;
    }

  private:
    int socket_fd_;
    std::string address_;
    uint16_t port_;
    PeerState state_;

    // Peer info
    uint32_t version_;
    uint32_t height_;
    uint64_t services_;
    uint64_t nonce_;
    std::string user_agent_;

    // Network magic
    uint32_t network_magic_;

    // Send/receive buffers
    std::vector<uint8_t> recv_buffer_;
    std::queue<std::vector<uint8_t>> send_queue_;
    std::mutex send_mutex_;

    // Callbacks
    std::function<void(const VersionMessage&)> on_version_;
    std::function<void()> on_verack_;
    std::function<void(uint64_t)> on_ping_;
    std::function<void(uint64_t)> on_pong_;
    std::function<void(const InvMessage&)> on_inv_;
    std::function<void(const GetDataMessage&)> on_getdata_;
    std::function<void(const primitives::Block&)> on_block_;
    std::function<void(const primitives::Transaction&)> on_tx_;
    std::function<void(const AddrMessage&)> on_addr_;
    std::function<void(const GetHeadersMessage&)> on_getheaders_;
    std::function<void(const HeadersMessage&)> on_headers_;

    // Internal helpers
    bool SendMessage(const char* command, const std::vector<uint8_t>& payload);
    bool SendRaw(const std::vector<uint8_t>& data);
    bool ReceiveRaw(size_t bytes);
    void ProcessMessage(const MessageHeader& header, const uint8_t* payload, size_t len);
};

/**
 * DNS seed for peer discovery
 */
struct DNSSeed {
    std::string hostname;
    uint16_t default_port;
};

/**
 * Network Manager - handles all P2P networking
 */
class NetworkManager {
  public:
    NetworkManager(uint16_t listen_port, uint32_t network_magic);
    ~NetworkManager();

    // Lifecycle
    bool Start();
    void Stop();
    bool IsRunning() const { return running_; }

    // Peer management
    void AddPeer(const std::string& address, uint16_t port);
    void RemovePeer(const std::string& peer_id);
    void BanPeer(const std::string& peer_id);
    std::vector<std::string> GetConnectedPeers() const;
    size_t GetPeerCount() const;

    // Peer discovery
    void AddDNSSeed(const std::string& hostname, uint16_t port);
    void QueryDNSSeeds();

    // Broadcasting
    void BroadcastBlock(const primitives::Block& block);
    void BroadcastTransaction(const primitives::Transaction& tx);
    void BroadcastInv(const InvMessage& inv);

    // Request data
    void RequestBlocks(const std::string& peer_id, uint32_t start_height, uint32_t count);
    void RequestHeaders(const std::string& peer_id,
                        const std::vector<std::array<uint8_t, 32>>& locator);

    // Callbacks for received messages
    void SetOnNewPeer(std::function<void(const std::string&)> callback) { on_new_peer_ = callback; }
    void SetOnBlock(std::function<void(const std::string&, const primitives::Block&)> callback) {
        on_block_ = callback;
    }
    void SetOnTransaction(
        std::function<void(const std::string&, const primitives::Transaction&)> callback) {
        on_transaction_ = callback;
    }
    void SetOnInv(std::function<void(const std::string&, const InvMessage&)> callback) {
        on_inv_ = callback;
    }
    void SetOnGetData(std::function<void(const std::string&, const GetDataMessage&)> callback) {
        on_getdata_ = callback;
    }
    void
    SetOnGetHeaders(std::function<void(const std::string&, const GetHeadersMessage&)> callback) {
        on_getheaders_ = callback;
    }
    void SetOnHeaders(std::function<void(const std::string&, const HeadersMessage&)> callback) {
        on_headers_ = callback;
    }

  private:
    uint16_t listen_port_;
    uint32_t network_magic_;
    std::atomic<bool> running_;

    // TCP listener
    int listen_socket_;
    std::thread accept_thread_;

    // Peer connections
    std::map<std::string, std::unique_ptr<PeerConnection>> peers_;
    mutable std::mutex peers_mutex_;

    // Connection threads
    std::vector<std::thread> connection_threads_;

    // DNS seeds
    std::vector<DNSSeed> dns_seeds_;

    // Banned peers
    std::set<std::string> banned_peers_;
    mutable std::mutex banned_mutex_;

    // Callbacks
    std::function<void(const std::string&)> on_new_peer_;
    std::function<void(const std::string&, const primitives::Block&)> on_block_;
    std::function<void(const std::string&, const primitives::Transaction&)> on_transaction_;
    std::function<void(const std::string&, const InvMessage&)> on_inv_;
    std::function<void(const std::string&, const GetDataMessage&)> on_getdata_;
    std::function<void(const std::string&, const GetHeadersMessage&)> on_getheaders_;
    std::function<void(const std::string&, const HeadersMessage&)> on_headers_;

    // Internal methods
    void AcceptLoop();
    void HandlePeer(const std::string& peer_id);
    bool CreateListenSocket();
    void ConnectOutbound(const std::string& address, uint16_t port);
    std::string MakePeerId(const std::string& address, uint16_t port);
    bool IsBanned(const std::string& address);
};

}  // namespace p2p
}  // namespace parthenon
