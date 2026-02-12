// ParthenonChain - Network Messages Implementation
// Security Review: Checking for buffer overflow and input validation issues

#include "message.h"

#include "crypto/sha256.h"

#include <cstring>
#include <stdexcept>

namespace parthenon {
namespace p2p {

// Helper to write compact size
static void WriteCompactSize(std::vector<uint8_t>& output, uint64_t size) {
    if (size < 253) {
        output.push_back(static_cast<uint8_t>(size));
    } else if (size <= 0xFFFF) {
        output.push_back(253);
        output.push_back(static_cast<uint8_t>(size));
        output.push_back(static_cast<uint8_t>(size >> 8));
    } else if (size <= 0xFFFFFFFF) {
        output.push_back(254);
        output.push_back(static_cast<uint8_t>(size));
        output.push_back(static_cast<uint8_t>(size >> 8));
        output.push_back(static_cast<uint8_t>(size >> 16));
        output.push_back(static_cast<uint8_t>(size >> 24));
    } else {
        output.push_back(255);
        for (int i = 0; i < 8; i++) {
            output.push_back(static_cast<uint8_t>(size >> (8 * i)));
        }
    }
}

// Helper to read compact size
static uint64_t ReadCompactSize(const uint8_t*& data) {
    uint8_t first = *data++;
    if (first < 253) {
        return first;
    } else if (first == 253) {
        uint64_t size = data[0] | (static_cast<uint64_t>(data[1]) << 8);
        data += 2;
        return size;
    } else if (first == 254) {
        uint64_t size = data[0] | (static_cast<uint64_t>(data[1]) << 8) |
                        (static_cast<uint64_t>(data[2]) << 16) |
                        (static_cast<uint64_t>(data[3]) << 24);
        data += 4;
        return size;
    } else {
        uint64_t size = 0;
        for (int i = 0; i < 8; i++) {
            size |= static_cast<uint64_t>(data[i]) << (8 * i);
        }
        data += 8;
        return size;
    }
}

// MessageHeader

std::vector<uint8_t> MessageHeader::Serialize() const {
    std::vector<uint8_t> result;
    result.reserve(24);

    // Magic (4 bytes)
    result.push_back(static_cast<uint8_t>(magic));
    result.push_back(static_cast<uint8_t>(magic >> 8));
    result.push_back(static_cast<uint8_t>(magic >> 16));
    result.push_back(static_cast<uint8_t>(magic >> 24));

    // Command (12 bytes)
    for (int i = 0; i < 12; i++) {
        result.push_back(static_cast<uint8_t>(command[i]));
    }

    // Length (4 bytes)
    result.push_back(static_cast<uint8_t>(length));
    result.push_back(static_cast<uint8_t>(length >> 8));
    result.push_back(static_cast<uint8_t>(length >> 16));
    result.push_back(static_cast<uint8_t>(length >> 24));

    // Checksum (4 bytes)
    result.push_back(static_cast<uint8_t>(checksum));
    result.push_back(static_cast<uint8_t>(checksum >> 8));
    result.push_back(static_cast<uint8_t>(checksum >> 16));
    result.push_back(static_cast<uint8_t>(checksum >> 24));

    return result;
}

std::optional<MessageHeader> MessageHeader::Deserialize(const uint8_t* data) {
    MessageHeader header;

    // Magic
    header.magic = data[0] | (static_cast<uint32_t>(data[1]) << 8) |
                   (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
    data += 4;

    // Command
    std::memcpy(header.command, data, 12);
    header.command[11] = '\0';  // Ensure null termination
    data += 12;

    // Length
    header.length = data[0] | (static_cast<uint32_t>(data[1]) << 8) |
                    (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
    data += 4;

    // Checksum
    header.checksum = data[0] | (static_cast<uint32_t>(data[1]) << 8) |
                      (static_cast<uint32_t>(data[2]) << 16) |
                      (static_cast<uint32_t>(data[3]) << 24);

    return header;
}

bool MessageHeader::IsValid(uint32_t expected_magic) const {
    if (magic != expected_magic) {
        return false;
    }
    if (length > MAX_MESSAGE_SIZE) {
        return false;
    }
    return true;
}

// VersionMessage

std::vector<uint8_t> VersionMessage::Serialize() const {
    std::vector<uint8_t> result;

    // Version (4 bytes)
    for (int i = 0; i < 4; i++) {
        result.push_back(static_cast<uint8_t>(version >> (8 * i)));
    }

    // Services (8 bytes)
    for (int i = 0; i < 8; i++) {
        result.push_back(static_cast<uint8_t>(services >> (8 * i)));
    }

    // Timestamp (8 bytes)
    for (int i = 0; i < 8; i++) {
        result.push_back(static_cast<uint8_t>(timestamp >> (8 * i)));
    }

    // Addr_recv (26 bytes) - simplified, just services + IP + port
    for (int i = 0; i < 8; i++) {
        result.push_back(static_cast<uint8_t>(addr_recv.services >> (8 * i)));
    }
    for (int i = 0; i < 16; i++) {
        result.push_back(addr_recv.ip[i]);
    }
    result.push_back(static_cast<uint8_t>(addr_recv.port >> 8));
    result.push_back(static_cast<uint8_t>(addr_recv.port));

    // Addr_from (26 bytes)
    for (int i = 0; i < 8; i++) {
        result.push_back(static_cast<uint8_t>(addr_from.services >> (8 * i)));
    }
    for (int i = 0; i < 16; i++) {
        result.push_back(addr_from.ip[i]);
    }
    result.push_back(static_cast<uint8_t>(addr_from.port >> 8));
    result.push_back(static_cast<uint8_t>(addr_from.port));

    // Nonce (8 bytes)
    for (int i = 0; i < 8; i++) {
        result.push_back(static_cast<uint8_t>(nonce >> (8 * i)));
    }

    // User agent
    WriteCompactSize(result, user_agent.size());
    for (char c : user_agent) {
        result.push_back(static_cast<uint8_t>(c));
    }

    // Start height (4 bytes)
    for (int i = 0; i < 4; i++) {
        result.push_back(static_cast<uint8_t>(start_height >> (8 * i)));
    }

    // Relay (1 byte)
    result.push_back(relay ? 1 : 0);

    return result;
}

std::optional<VersionMessage> VersionMessage::Deserialize(const uint8_t* data, size_t len) {
    if (len < 85)
        return std::nullopt;  // Minimum size

    VersionMessage msg;
    const uint8_t* ptr = data;

    // Version
    msg.version = ptr[0] | (static_cast<uint32_t>(ptr[1]) << 8) |
                  (static_cast<uint32_t>(ptr[2]) << 16) | (static_cast<uint32_t>(ptr[3]) << 24);
    ptr += 4;

    // Services
    msg.services = 0;
    for (int i = 0; i < 8; i++) {
        msg.services |= static_cast<uint64_t>(ptr[i]) << (8 * i);
    }
    ptr += 8;

    // Timestamp
    msg.timestamp = 0;
    for (int i = 0; i < 8; i++) {
        msg.timestamp |= static_cast<int64_t>(ptr[i]) << (8 * i);
    }
    ptr += 8;

    // Skip addr_recv and addr_from for now (52 bytes total)
    ptr += 52;

    // Nonce
    msg.nonce = 0;
    for (int i = 0; i < 8; i++) {
        msg.nonce |= static_cast<uint64_t>(ptr[i]) << (8 * i);
    }
    ptr += 8;

    // User agent
    uint64_t ua_len = ReadCompactSize(ptr);
    if (ua_len > 256 || ptr + ua_len > data + len)
        return std::nullopt;  // Bounds check
    msg.user_agent = std::string(reinterpret_cast<const char*>(ptr), ua_len);
    ptr += ua_len;

    // Start height
    msg.start_height = ptr[0] | (static_cast<uint32_t>(ptr[1]) << 8) |
                       (static_cast<uint32_t>(ptr[2]) << 16) |
                       (static_cast<uint32_t>(ptr[3]) << 24);
    ptr += 4;

    // Relay (optional)
    if (ptr < data + len) {
        msg.relay = (*ptr != 0);
    }

    return msg;
}

// PingPongMessage

std::vector<uint8_t> PingPongMessage::Serialize() const {
    std::vector<uint8_t> result;
    for (int i = 0; i < 8; i++) {
        result.push_back(static_cast<uint8_t>(nonce >> (8 * i)));
    }
    return result;
}

std::optional<PingPongMessage> PingPongMessage::Deserialize(const uint8_t* data, size_t len) {
    if (len < 8)
        return std::nullopt;

    PingPongMessage msg;
    msg.nonce = 0;
    for (int i = 0; i < 8; i++) {
        msg.nonce |= static_cast<uint64_t>(data[i]) << (8 * i);
    }
    return msg;
}

// InvVect

std::vector<uint8_t> InvVect::Serialize() const {
    std::vector<uint8_t> result;
    result.reserve(36);  // 4 bytes for type + 32 bytes for hash

    // Type (4 bytes)
    uint32_t type_val = static_cast<uint32_t>(type);
    for (int i = 0; i < 4; i++) {
        result.push_back(static_cast<uint8_t>(type_val >> (8 * i)));
    }

    // Hash (32 bytes)
    result.insert(result.end(), hash.begin(), hash.end());

    return result;
}

std::optional<InvVect> InvVect::Deserialize(const uint8_t* data) {
    InvVect inv;

    // Type
    uint32_t type_val = data[0] | (static_cast<uint32_t>(data[1]) << 8) |
                        (static_cast<uint32_t>(data[2]) << 16) |
                        (static_cast<uint32_t>(data[3]) << 24);
    inv.type = static_cast<InvType>(type_val);
    data += 4;

    // Hash
    std::copy(data, data + 32, inv.hash.begin());

    return inv;
}

// InvMessage

std::vector<uint8_t> InvMessage::Serialize() const {
    std::vector<uint8_t> result;

    WriteCompactSize(result, inventory.size());
    for (const auto& inv : inventory) {
        auto inv_bytes = inv.Serialize();
        result.insert(result.end(), inv_bytes.begin(), inv_bytes.end());
    }

    return result;
}

std::optional<InvMessage> InvMessage::Deserialize(const uint8_t* data, size_t len) {
    InvMessage msg;
    const uint8_t* ptr = data;

    uint64_t count = ReadCompactSize(ptr);
    if (count > MAX_INV_SIZE)
        return std::nullopt;

    for (uint64_t i = 0; i < count; i++) {
        if (ptr + 36 > data + len)
            return std::nullopt;

        auto inv = InvVect::Deserialize(ptr);
        if (!inv)
            return std::nullopt;

        msg.inventory.push_back(*inv);
        ptr += 36;
    }

    return msg;
}

// AddrMessage

std::vector<uint8_t> AddrMessage::Serialize() const {
    throw std::logic_error("AddrMessage::Serialize not implemented");
}

std::optional<AddrMessage> AddrMessage::Deserialize(const uint8_t* data, size_t len) {
    (void)data;
    (void)len;
    throw std::logic_error("AddrMessage::Deserialize not implemented");
}

// BlockMessage

std::vector<uint8_t> BlockMessage::Serialize() const {
    throw std::logic_error("BlockMessage::Serialize not implemented");
}

std::optional<BlockMessage> BlockMessage::Deserialize(const uint8_t* data, size_t len) {
    (void)data;
    (void)len;
    throw std::logic_error("BlockMessage::Deserialize not implemented");
}

// TxMessage

std::vector<uint8_t> TxMessage::Serialize() const {
    throw std::logic_error("TxMessage::Serialize not implemented");
}

std::optional<TxMessage> TxMessage::Deserialize(const uint8_t* data, size_t len) {
    (void)data;
    (void)len;
    throw std::logic_error("TxMessage::Deserialize not implemented");
}

// GetHeadersMessage

std::vector<uint8_t> GetHeadersMessage::Serialize() const {
    throw std::logic_error("GetHeadersMessage::Serialize not implemented");
}

std::optional<GetHeadersMessage> GetHeadersMessage::Deserialize(const uint8_t* data, size_t len) {
    (void)data;
    (void)len;
    throw std::logic_error("GetHeadersMessage::Deserialize not implemented");
}

// HeadersMessage

std::vector<uint8_t> HeadersMessage::Serialize() const {
    throw std::logic_error("HeadersMessage::Serialize not implemented");
}

std::optional<HeadersMessage> HeadersMessage::Deserialize(const uint8_t* data, size_t len) {
    (void)data;
    (void)len;
    throw std::logic_error("HeadersMessage::Deserialize not implemented");
}

// Helper functions

uint32_t CalculateChecksum(const std::vector<uint8_t>& payload) {
    auto hash = crypto::SHA256d::Hash256d(payload);
    return hash[0] | (static_cast<uint32_t>(hash[1]) << 8) |
           (static_cast<uint32_t>(hash[2]) << 16) | (static_cast<uint32_t>(hash[3]) << 24);
}

std::vector<uint8_t> CreateNetworkMessage(uint32_t magic, const char* command,
                                          const std::vector<uint8_t>& payload) {
    MessageHeader header;
    header.magic = magic;
    // Safe string copy with null termination
    std::memset(header.command, 0, 12);  // Zero the entire buffer (ensures null termination)
    size_t len = std::strlen(command);
    if (len >= 12)
        len = 11;  // Leave room for null terminator
    std::memcpy(header.command, command, len);
    header.length = static_cast<uint32_t>(payload.size());
    header.checksum = CalculateChecksum(payload);

    auto result = header.Serialize();
    result.insert(result.end(), payload.begin(), payload.end());

    return result;
}

}  // namespace p2p
}  // namespace parthenon
