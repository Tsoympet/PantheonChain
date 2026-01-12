// ParthenonChain - Network Messages
// P2P message serialization and deserialization

#ifndef PARTHENON_P2P_MESSAGE_H
#define PARTHENON_P2P_MESSAGE_H

#include "protocol.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include <vector>
#include <array>
#include <optional>
#include <string>

namespace parthenon {
namespace p2p {

/**
 * Message header (24 bytes)
 */
struct MessageHeader {
    uint32_t magic;           // Network magic bytes
    char command[12];         // Command name (null-padded)
    uint32_t length;          // Payload length
    uint32_t checksum;        // First 4 bytes of SHA256d(payload)
    
    MessageHeader() : magic(0), command{}, length(0), checksum(0) {}
    
    /**
     * Serialize header to bytes
     */
    std::vector<uint8_t> Serialize() const;
    
    /**
     * Deserialize header from bytes
     */
    static std::optional<MessageHeader> Deserialize(const uint8_t* data);
    
    /**
     * Validate header
     */
    bool IsValid(uint32_t expected_magic) const;
};

/**
 * Version message payload
 */
struct VersionMessage {
    uint32_t version;           // Protocol version
    uint64_t services;          // Service flags
    int64_t timestamp;          // Current time
    NetAddr addr_recv;          // Receiver's address
    NetAddr addr_from;          // Sender's address
    uint64_t nonce;             // Random nonce
    std::string user_agent;     // Client identifier
    uint32_t start_height;      // Sender's blockchain height
    bool relay;                 // Whether to relay transactions
    
    VersionMessage()
        : version(PROTOCOL_VERSION), services(0), timestamp(0),
          addr_recv(), addr_from(), nonce(0), user_agent(),
          start_height(0), relay(true) {}
    
    std::vector<uint8_t> Serialize() const;
    static std::optional<VersionMessage> Deserialize(const uint8_t* data, size_t len);
};

/**
 * Ping/Pong message
 */
struct PingPongMessage {
    uint64_t nonce;
    
    PingPongMessage() : nonce(0) {}
    explicit PingPongMessage(uint64_t n) : nonce(n) {}
    
    std::vector<uint8_t> Serialize() const;
    static std::optional<PingPongMessage> Deserialize(const uint8_t* data, size_t len);
};

/**
 * Inventory vector
 */
struct InvVect {
    InvType type;
    std::array<uint8_t, 32> hash;
    
    InvVect() : type(InvType::ERROR), hash{} {}
    InvVect(InvType t, const std::array<uint8_t, 32>& h) : type(t), hash(h) {}
    
    std::vector<uint8_t> Serialize() const;
    static std::optional<InvVect> Deserialize(const uint8_t* data);
};

/**
 * Inv message payload
 */
struct InvMessage {
    std::vector<InvVect> inventory;
    
    InvMessage() = default;
    
    std::vector<uint8_t> Serialize() const;
    static std::optional<InvMessage> Deserialize(const uint8_t* data, size_t len);
};

/**
 * GetData message (same format as Inv)
 */
using GetDataMessage = InvMessage;

/**
 * Addr message payload
 */
struct AddrMessage {
    std::vector<NetAddr> addresses;
    
    AddrMessage() = default;
    
    std::vector<uint8_t> Serialize() const;
    static std::optional<AddrMessage> Deserialize(const uint8_t* data, size_t len);
};

/**
 * Block message payload
 */
struct BlockMessage {
    primitives::Block block;
    
    BlockMessage() = default;
    explicit BlockMessage(const primitives::Block& b) : block(b) {}
    
    std::vector<uint8_t> Serialize() const;
    static std::optional<BlockMessage> Deserialize(const uint8_t* data, size_t len);
};

/**
 * Tx message payload
 */
struct TxMessage {
    primitives::Transaction tx;
    
    TxMessage() = default;
    explicit TxMessage(const primitives::Transaction& t) : tx(t) {}
    
    std::vector<uint8_t> Serialize() const;
    static std::optional<TxMessage> Deserialize(const uint8_t* data, size_t len);
};

/**
 * GetHeaders/GetBlocks message payload
 */
struct GetHeadersMessage {
    uint32_t version;
    std::vector<std::array<uint8_t, 32>> block_locator_hashes;
    std::array<uint8_t, 32> hash_stop;
    
    GetHeadersMessage() : version(PROTOCOL_VERSION), hash_stop{} {}
    
    std::vector<uint8_t> Serialize() const;
    static std::optional<GetHeadersMessage> Deserialize(const uint8_t* data, size_t len);
};

/**
 * Headers message payload
 */
struct HeadersMessage {
    std::vector<primitives::BlockHeader> headers;
    
    HeadersMessage() = default;
    
    std::vector<uint8_t> Serialize() const;
    static std::optional<HeadersMessage> Deserialize(const uint8_t* data, size_t len);
};

/**
 * Reject message payload
 */
struct RejectMessage {
    std::string message;        // Message being rejected
    uint8_t ccode;              // Reject code
    std::string reason;         // Reason for rejection
    std::vector<uint8_t> data;  // Extra data (e.g., tx/block hash)
    
    RejectMessage() : ccode(0) {}
    
    std::vector<uint8_t> Serialize() const;
    static std::optional<RejectMessage> Deserialize(const uint8_t* data, size_t len);
};

/**
 * Helper functions
 */

// Calculate message checksum
uint32_t CalculateChecksum(const std::vector<uint8_t>& payload);

// Create a complete network message
std::vector<uint8_t> CreateNetworkMessage(
    uint32_t magic,
    const char* command,
    const std::vector<uint8_t>& payload
);

} // namespace p2p
} // namespace parthenon

#endif // PARTHENON_P2P_MESSAGE_H
