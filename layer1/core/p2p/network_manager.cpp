// ParthenonChain - P2P Network Manager Implementation

#include "network_manager.h"

#include "crypto/sha256.h"

#include <chrono>
#include <cstring>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#endif
#include <iostream>
#include <random>

namespace parthenon {
namespace p2p {

// PeerConnection implementation

PeerConnection::PeerConnection(int socket_fd, const std::string& address, uint16_t port)
    : socket_fd_(socket_fd),
      address_(address),
      port_(port),
      state_(PeerState::CONNECTING),
      version_(0),
      height_(0),
      services_(0),
      nonce_(0),
      network_magic_(NetworkMagic::MAINNET) {
    // Generate random nonce for this connection
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    nonce_ = dis(gen);
}

PeerConnection::~PeerConnection() {
    Disconnect();
}

bool PeerConnection::Connect() {
    if (socket_fd_ < 0) {
        // Create socket
        socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd_ < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }

        // Set non-blocking
        int flags = fcntl(socket_fd_, F_GETFL, 0);
        fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK);

        // Connect to peer
        struct sockaddr_in server_addr {};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);

        if (inet_pton(AF_INET, address_.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address: " << address_ << std::endl;
            close(socket_fd_);
            socket_fd_ = -1;
            return false;
        }

        int result = connect(socket_fd_,
                             reinterpret_cast<struct sockaddr*>(&server_addr),
                             sizeof(server_addr));
        if (result < 0 && errno != EINPROGRESS) {
            std::cerr << "Failed to connect to " << address_ << ":" << port_ << std::endl;
            close(socket_fd_);
            socket_fd_ = -1;
            return false;
        }
    }

    state_ = PeerState::HANDSHAKE;
    return true;
}

void PeerConnection::Disconnect() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    state_ = PeerState::DISCONNECTED;
}

bool PeerConnection::SendMessage(const char* command, const std::vector<uint8_t>& payload) {
    auto message = CreateNetworkMessage(network_magic_, command, payload);
    return SendRaw(message);
}

bool PeerConnection::SendRaw(const std::vector<uint8_t>& data) {
    if (socket_fd_ < 0 || state_ == PeerState::DISCONNECTED) {
        return false;
    }

    std::lock_guard<std::mutex> lock(send_mutex_);

    size_t sent = 0;
    while (sent < data.size()) {
        ssize_t result = send(socket_fd_, data.data() + sent, data.size() - sent, 0);
        if (result < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Would block, queue for later
                send_queue_.push(std::vector<uint8_t>(data.begin() + sent, data.end()));
                return true;
            }
            std::cerr << "Send failed: " << strerror(errno) << std::endl;
            return false;
        }
        sent += result;
    }

    return true;
}

bool PeerConnection::ReceiveRaw(size_t bytes) {
    std::vector<uint8_t> buffer(bytes);

    ssize_t received = recv(socket_fd_, buffer.data(), bytes, 0);
    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return true;  // No data available yet
        }
        std::cerr << "Receive failed: " << strerror(errno) << std::endl;
        return false;
    }

    if (received == 0) {
        // Connection closed
        state_ = PeerState::DISCONNECTED;
        return false;
    }

    recv_buffer_.insert(recv_buffer_.end(), buffer.begin(), buffer.begin() + received);
    return true;
}

bool PeerConnection::ReceiveMessage() {
    // Ensure we have at least a header
    while (recv_buffer_.size() < 24) {
        if (!ReceiveRaw(1024)) {
            return false;
        }
    }

    // Parse header
    auto header_opt = MessageHeader::Deserialize(recv_buffer_.data());
    if (!header_opt) {
        std::cerr << "Invalid message header" << std::endl;
        return false;
    }

    auto header = *header_opt;

    // Validate header
    if (!header.IsValid(network_magic_)) {
        std::cerr << "Invalid message magic" << std::endl;
        return false;
    }

    // Wait for full message
    size_t total_size = 24 + header.length;
    while (recv_buffer_.size() < total_size) {
        if (!ReceiveRaw(4096)) {
            return false;
        }
    }

    // Extract payload
    const uint8_t* payload = recv_buffer_.data() + 24;

    // Process message
    ProcessMessage(header, payload, header.length);

    // Remove processed message from buffer
    recv_buffer_.erase(recv_buffer_.begin(), recv_buffer_.begin() + total_size);

    return true;
}

void PeerConnection::ProcessMessage(const MessageHeader& header, const uint8_t* payload,
                                    size_t len) {
    std::string command(header.command);

    if (command == "version") {
        auto msg = VersionMessage::Deserialize(payload, len);
        if (msg && on_version_) {
            version_ = msg->version;
            height_ = msg->start_height;
            services_ = msg->services;
            user_agent_ = msg->user_agent;
            on_version_(*msg);
        }
    } else if (command == "verack") {
        state_ = PeerState::CONNECTED;
        if (on_verack_) {
            on_verack_();
        }
    } else if (command == "ping") {
        auto msg = PingPongMessage::Deserialize(payload, len);
        if (msg && on_ping_) {
            on_ping_(msg->nonce);
        }
    } else if (command == "pong") {
        auto msg = PingPongMessage::Deserialize(payload, len);
        if (msg && on_pong_) {
            on_pong_(msg->nonce);
        }
    } else if (command == "inv") {
        auto msg = InvMessage::Deserialize(payload, len);
        if (msg && on_inv_) {
            on_inv_(*msg);
        }
    } else if (command == "getdata") {
        auto msg = GetDataMessage::Deserialize(payload, len);
        if (msg && on_getdata_) {
            on_getdata_(*msg);
        }
    } else if (command == "block") {
        auto msg = BlockMessage::Deserialize(payload, len);
        if (msg && on_block_) {
            on_block_(msg->block);
        }
    } else if (command == "tx") {
        auto msg = TxMessage::Deserialize(payload, len);
        if (msg && on_tx_) {
            on_tx_(msg->tx);
        }
    } else if (command == "addr") {
        auto msg = AddrMessage::Deserialize(payload, len);
        if (msg && on_addr_) {
            on_addr_(*msg);
        }
    } else if (command == "getheaders") {
        auto msg = GetHeadersMessage::Deserialize(payload, len);
        if (msg && on_getheaders_) {
            on_getheaders_(*msg);
        }
    } else if (command == "headers") {
        auto msg = HeadersMessage::Deserialize(payload, len);
        if (msg && on_headers_) {
            on_headers_(*msg);
        }
    }
}

bool PeerConnection::SendVersion(const VersionMessage& msg) {
    return SendMessage("version", msg.Serialize());
}

bool PeerConnection::SendVerack() {
    return SendMessage("verack", {});
}

bool PeerConnection::SendPing(uint64_t nonce) {
    PingPongMessage msg(nonce);
    return SendMessage("ping", msg.Serialize());
}

bool PeerConnection::SendPong(uint64_t nonce) {
    PingPongMessage msg(nonce);
    return SendMessage("pong", msg.Serialize());
}

bool PeerConnection::SendGetHeaders(const GetHeadersMessage& msg) {
    return SendMessage("getheaders", msg.Serialize());
}

bool PeerConnection::SendGetData(const GetDataMessage& msg) {
    return SendMessage("getdata", msg.Serialize());
}

bool PeerConnection::SendInv(const InvMessage& msg) {
    return SendMessage("inv", msg.Serialize());
}

bool PeerConnection::SendBlock(const primitives::Block& block) {
    BlockMessage msg(block);
    return SendMessage("block", msg.Serialize());
}

bool PeerConnection::SendTx(const primitives::Transaction& tx) {
    TxMessage msg(tx);
    return SendMessage("tx", msg.Serialize());
}

bool PeerConnection::SendAddr(const AddrMessage& msg) {
    return SendMessage("addr", msg.Serialize());
}

// NetworkManager implementation

NetworkManager::NetworkManager(uint16_t listen_port, uint32_t network_magic)
    : listen_port_(listen_port),
      network_magic_(network_magic),
      running_(false),
      listen_socket_(-1) {}

NetworkManager::~NetworkManager() {
    Stop();
}

bool NetworkManager::Start() {
    if (running_) {
        return false;
    }

    std::cout << "Starting P2P network manager on port " << listen_port_ << std::endl;

    // Create listener socket
    if (!CreateListenSocket()) {
        std::cerr << "Failed to create listen socket" << std::endl;
        return false;
    }

    running_ = true;

    // Start accept thread
    accept_thread_ = std::thread(&NetworkManager::AcceptLoop, this);

    std::cout << "P2P network manager started successfully" << std::endl;
    return true;
}

void NetworkManager::Stop() {
    if (!running_) {
        return;
    }

    std::cout << "Stopping P2P network manager..." << std::endl;

    running_ = false;

    // Close listen socket
    if (listen_socket_ >= 0) {
        close(listen_socket_);
        listen_socket_ = -1;
    }

    // Wait for accept thread
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }

    // Disconnect all peers
    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        peers_.clear();
    }

    // Wait for connection threads
    for (auto& thread : connection_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    connection_threads_.clear();

    std::cout << "P2P network manager stopped" << std::endl;
}

bool NetworkManager::CreateListenSocket() {
    listen_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket_ < 0) {
        return false;
    }

    // Set socket options
    int opt = 1;
    setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to port
    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(listen_port_);

    if (bind(listen_socket_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(listen_socket_);
        listen_socket_ = -1;
        return false;
    }

    // Listen for connections
    if (listen(listen_socket_, MAX_INBOUND_CONNECTIONS) < 0) {
        close(listen_socket_);
        listen_socket_ = -1;
        return false;
    }

    return true;
}

void NetworkManager::AcceptLoop() {
    while (running_) {
        struct sockaddr_in client_addr {};
        socklen_t addr_len = sizeof(client_addr);

        int client_socket =
            accept(listen_socket_, reinterpret_cast<struct sockaddr*>(&client_addr), &addr_len);
        if (client_socket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            break;
        }

        // Get peer address
        char addr_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, INET_ADDRSTRLEN);
        std::string address(addr_str);
        uint16_t port = ntohs(client_addr.sin_port);

        // Check if banned
        if (IsBanned(address)) {
            close(client_socket);
            continue;
        }

        // Create peer connection
        std::string peer_id = MakePeerId(address, port);

        {
            std::lock_guard<std::mutex> lock(peers_mutex_);
            if (peers_.size() >= MAX_CONNECTIONS) {
                close(client_socket);
                continue;
            }

            auto peer = std::make_unique<PeerConnection>(client_socket, address, port);
            peers_[peer_id] = std::move(peer);
        }

        // Start handler thread
        connection_threads_.emplace_back(&NetworkManager::HandlePeer, this, peer_id);

        if (on_new_peer_) {
            on_new_peer_(peer_id);
        }
    }
}

void NetworkManager::HandlePeer(const std::string& peer_id) {
    std::unique_ptr<PeerConnection> peer;

    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        auto it = peers_.find(peer_id);
        if (it == peers_.end()) {
            return;
        }
        peer = std::move(it->second);
        peers_.erase(it);
    }

    // Set up callbacks
    peer->SetOnBlock([this, peer_id](const primitives::Block& block) {
        if (on_block_) {
            on_block_(peer_id, block);
        }
    });

    peer->SetOnTx([this, peer_id](const primitives::Transaction& tx) {
        if (on_transaction_) {
            on_transaction_(peer_id, tx);
        }
    });

    peer->SetOnInv([this, peer_id](const InvMessage& inv) {
        if (on_inv_) {
            on_inv_(peer_id, inv);
        }
    });

    peer->SetOnGetData([this, peer_id](const GetDataMessage& msg) {
        if (on_getdata_) {
            on_getdata_(peer_id, msg);
        }
    });

    peer->SetOnGetHeaders([this, peer_id](const GetHeadersMessage& msg) {
        if (on_getheaders_) {
            on_getheaders_(peer_id, msg);
        }
    });

    peer->SetOnHeaders([this, peer_id](const HeadersMessage& msg) {
        if (on_headers_) {
            on_headers_(peer_id, msg);
        }
    });

    peer->SetOnPing([&peer](uint64_t nonce) { peer->SendPong(nonce); });

    // Message receive loop
    while (running_ && peer->IsConnected()) {
        if (!peer->ReceiveMessage()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    peer->Disconnect();
}

void NetworkManager::AddPeer(const std::string& address, uint16_t port) {
    if (IsBanned(address)) {
        return;
    }

    connection_threads_.emplace_back(&NetworkManager::ConnectOutbound, this, address, port);
}

void NetworkManager::ConnectOutbound(const std::string& address, uint16_t port) {
    std::string peer_id = MakePeerId(address, port);

    auto peer = std::make_unique<PeerConnection>(-1, address, port);
    if (!peer->Connect()) {
        std::cerr << "Failed to connect to " << address << ":" << port << std::endl;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        peers_[peer_id] = std::move(peer);
    }

    HandlePeer(peer_id);
}

void NetworkManager::RemovePeer(const std::string& peer_id) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    peers_.erase(peer_id);
}

void NetworkManager::BanPeer(const std::string& peer_id) {
    // Extract address from peer_id
    size_t colon_pos = peer_id.find(':');
    if (colon_pos != std::string::npos) {
        std::string address = peer_id.substr(0, colon_pos);
        std::lock_guard<std::mutex> lock(banned_mutex_);
        banned_peers_.insert(address);
    }
    RemovePeer(peer_id);
}

std::vector<std::string> NetworkManager::GetConnectedPeers() const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    std::vector<std::string> result;
    for (const auto& [peer_id, peer] : peers_) {
        if (peer->IsConnected()) {
            result.push_back(peer_id);
        }
    }
    return result;
}

size_t NetworkManager::GetPeerCount() const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    size_t count = 0;
    for (const auto& [_, peer] : peers_) {
        if (peer->IsConnected()) {
            count++;
        }
    }
    return count;
}

void NetworkManager::BroadcastBlock(const primitives::Block& block) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    for (const auto& [peer_id, peer] : peers_) {
        if (peer->IsConnected()) {
            peer->SendBlock(block);
        }
    }
}

void NetworkManager::BroadcastTransaction(const primitives::Transaction& tx) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    for (const auto& [peer_id, peer] : peers_) {
        if (peer->IsConnected()) {
            peer->SendTx(tx);
        }
    }
}

void NetworkManager::BroadcastInv(const InvMessage& inv) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    for (const auto& [peer_id, peer] : peers_) {
        if (peer->IsConnected()) {
            peer->SendInv(inv);
        }
    }
}

void NetworkManager::RequestBlocks(const std::string& peer_id, uint32_t start_height,
                                   uint32_t count) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    auto it = peers_.find(peer_id);
    if (it != peers_.end() && it->second->IsConnected()) {
        // Create GetHeaders message
        GetHeadersMessage msg;
        // Would need to populate with actual block locator
        it->second->SendGetHeaders(msg);
    }
}

void NetworkManager::RequestHeaders(const std::string& peer_id,
                                    const std::vector<std::array<uint8_t, 32>>& locator) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    auto it = peers_.find(peer_id);
    if (it != peers_.end() && it->second->IsConnected()) {
        GetHeadersMessage msg;
        msg.block_locator_hashes = locator;
        it->second->SendGetHeaders(msg);
    }
}

void NetworkManager::AddDNSSeed(const std::string& hostname, uint16_t port) {
    dns_seeds_.push_back({hostname, port});
}

void NetworkManager::QueryDNSSeeds() {
    for (const auto& seed : dns_seeds_) {
        struct addrinfo hints {
        }, *result;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(seed.hostname.c_str(), nullptr, &hints, &result) == 0) {
            for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
                if (rp->ai_family == AF_INET) {
                    struct sockaddr_in* ipv4 =
                        reinterpret_cast<struct sockaddr_in*>(rp->ai_addr);
                    char addr_str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &ipv4->sin_addr, addr_str, INET_ADDRSTRLEN);
                    AddPeer(addr_str, seed.default_port);
                }
            }
            freeaddrinfo(result);
        }
    }
}

std::string NetworkManager::MakePeerId(const std::string& address, uint16_t port) {
    return address + ":" + std::to_string(port);
}

bool NetworkManager::IsBanned(const std::string& address) {
    std::lock_guard<std::mutex> lock(banned_mutex_);
    return banned_peers_.find(address) != banned_peers_.end();
}

}  // namespace p2p
}  // namespace parthenon
