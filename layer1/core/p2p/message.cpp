// ParthenonChain - Network Messages Implementation

#include "message.h"

#include "crypto/sha256.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace parthenon {
namespace p2p {

namespace {

constexpr size_t kHeaderSize = 104;  // primitives::BlockHeader serialized size

void WriteCompactSize(std::vector<uint8_t>& output, uint64_t size) {
    if (size < 253) {
        output.push_back(static_cast<uint8_t>(size));
    } else if (size <= 0xFFFF) {
        output.push_back(253);
        output.push_back(static_cast<uint8_t>(size));
        output.push_back(static_cast<uint8_t>(size >> 8));
    } else if (size <= 0xFFFFFFFFULL) {
        output.push_back(254);
        output.push_back(static_cast<uint8_t>(size));
        output.push_back(static_cast<uint8_t>(size >> 8));
        output.push_back(static_cast<uint8_t>(size >> 16));
        output.push_back(static_cast<uint8_t>(size >> 24));
    } else {
        output.push_back(255);
        for (int i = 0; i < 8; ++i) {
            output.push_back(static_cast<uint8_t>(size >> (8 * i)));
        }
    }
}

std::optional<uint64_t> ReadCompactSizeChecked(const uint8_t*& data, const uint8_t* end) {
    if (data >= end) {
        return std::nullopt;
    }

    const uint8_t first = *data++;
    if (first < 253) {
        return first;
    }

    if (first == 253) {
        if (end - data < 2) {
            return std::nullopt;
        }
        const uint64_t size = data[0] | (static_cast<uint64_t>(data[1]) << 8);
        data += 2;
        return size;
    }

    if (first == 254) {
        if (end - data < 4) {
            return std::nullopt;
        }
        const uint64_t size = data[0] | (static_cast<uint64_t>(data[1]) << 8) |
                              (static_cast<uint64_t>(data[2]) << 16) |
                              (static_cast<uint64_t>(data[3]) << 24);
        data += 4;
        return size;
    }

    if (end - data < 8) {
        return std::nullopt;
    }

    uint64_t size = 0;
    for (int i = 0; i < 8; ++i) {
        size |= static_cast<uint64_t>(data[i]) << (8 * i);
    }
    data += 8;
    return size;
}

void SerializeNetAddr(std::vector<uint8_t>& out, const NetAddr& addr, bool include_time) {
    if (include_time) {
        out.push_back(static_cast<uint8_t>(addr.time));
        out.push_back(static_cast<uint8_t>(addr.time >> 8));
        out.push_back(static_cast<uint8_t>(addr.time >> 16));
        out.push_back(static_cast<uint8_t>(addr.time >> 24));
    }

    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<uint8_t>(addr.services >> (8 * i)));
    }

    out.insert(out.end(), std::begin(addr.ip), std::end(addr.ip));
    out.push_back(static_cast<uint8_t>(addr.port >> 8));
    out.push_back(static_cast<uint8_t>(addr.port));
}

bool DeserializeNetAddr(const uint8_t*& ptr, const uint8_t* end, NetAddr& addr, bool include_time) {
    if (include_time) {
        if (end - ptr < 4) {
            return false;
        }
        addr.time = ptr[0] | (static_cast<uint32_t>(ptr[1]) << 8) |
                    (static_cast<uint32_t>(ptr[2]) << 16) |
                    (static_cast<uint32_t>(ptr[3]) << 24);
        ptr += 4;
    }

    if (end - ptr < 26) {
        return false;
    }

    addr.services = 0;
    for (int i = 0; i < 8; ++i) {
        addr.services |= static_cast<uint64_t>(ptr[i]) << (8 * i);
    }
    ptr += 8;

    std::copy(ptr, ptr + 16, addr.ip);
    ptr += 16;

    addr.port = static_cast<uint16_t>((static_cast<uint16_t>(ptr[0]) << 8) |
                                      static_cast<uint16_t>(ptr[1]));
    ptr += 2;

    return true;
}

}  // namespace

std::vector<uint8_t> MessageHeader::Serialize() const {
    std::vector<uint8_t> result;
    result.reserve(24);

    result.push_back(static_cast<uint8_t>(magic));
    result.push_back(static_cast<uint8_t>(magic >> 8));
    result.push_back(static_cast<uint8_t>(magic >> 16));
    result.push_back(static_cast<uint8_t>(magic >> 24));

    for (int i = 0; i < 12; ++i) {
        result.push_back(static_cast<uint8_t>(command[i]));
    }

    result.push_back(static_cast<uint8_t>(length));
    result.push_back(static_cast<uint8_t>(length >> 8));
    result.push_back(static_cast<uint8_t>(length >> 16));
    result.push_back(static_cast<uint8_t>(length >> 24));

    result.push_back(static_cast<uint8_t>(checksum));
    result.push_back(static_cast<uint8_t>(checksum >> 8));
    result.push_back(static_cast<uint8_t>(checksum >> 16));
    result.push_back(static_cast<uint8_t>(checksum >> 24));

    return result;
}

std::optional<MessageHeader> MessageHeader::Deserialize(const uint8_t* data) {
    if (data == nullptr) {
        return std::nullopt;
    }

    MessageHeader header;

    header.magic = data[0] | (static_cast<uint32_t>(data[1]) << 8) |
                   (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
    data += 4;

    std::memcpy(header.command, data, 12);
    data += 12;

    header.length = data[0] | (static_cast<uint32_t>(data[1]) << 8) |
                    (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
    data += 4;

    header.checksum = data[0] | (static_cast<uint32_t>(data[1]) << 8) |
                      (static_cast<uint32_t>(data[2]) << 16) |
                      (static_cast<uint32_t>(data[3]) << 24);

    return header;
}

bool MessageHeader::IsValid(uint32_t expected_magic) const {
    return magic == expected_magic && length <= MAX_MESSAGE_SIZE;
}

std::vector<uint8_t> VersionMessage::Serialize() const {
    std::vector<uint8_t> result;

    for (int i = 0; i < 4; ++i) {
        result.push_back(static_cast<uint8_t>(version >> (8 * i)));
    }
    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>(services >> (8 * i)));
    }
    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>(timestamp >> (8 * i)));
    }

    SerializeNetAddr(result, addr_recv, false);
    SerializeNetAddr(result, addr_from, false);

    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>(nonce >> (8 * i)));
    }

    WriteCompactSize(result, user_agent.size());
    result.insert(result.end(), user_agent.begin(), user_agent.end());

    for (int i = 0; i < 4; ++i) {
        result.push_back(static_cast<uint8_t>(start_height >> (8 * i)));
    }

    result.push_back(relay ? 1 : 0);
    return result;
}

std::optional<VersionMessage> VersionMessage::Deserialize(const uint8_t* data, size_t len) {
    if (data == nullptr || len < 85) {
        return std::nullopt;
    }

    VersionMessage msg;
    const uint8_t* ptr = data;
    const uint8_t* end = data + len;

    msg.version = ptr[0] | (static_cast<uint32_t>(ptr[1]) << 8) |
                  (static_cast<uint32_t>(ptr[2]) << 16) | (static_cast<uint32_t>(ptr[3]) << 24);
    ptr += 4;

    msg.services = 0;
    for (int i = 0; i < 8; ++i) {
        msg.services |= static_cast<uint64_t>(ptr[i]) << (8 * i);
    }
    ptr += 8;

    msg.timestamp = 0;
    for (int i = 0; i < 8; ++i) {
        msg.timestamp |= static_cast<int64_t>(ptr[i]) << (8 * i);
    }
    ptr += 8;

    if (!DeserializeNetAddr(ptr, end, msg.addr_recv, false) ||
        !DeserializeNetAddr(ptr, end, msg.addr_from, false)) {
        return std::nullopt;
    }

    if (end - ptr < 8) {
        return std::nullopt;
    }
    msg.nonce = 0;
    for (int i = 0; i < 8; ++i) {
        msg.nonce |= static_cast<uint64_t>(ptr[i]) << (8 * i);
    }
    ptr += 8;

    auto ua_size = ReadCompactSizeChecked(ptr, end);
    if (!ua_size || *ua_size > 256 || end - ptr < static_cast<std::ptrdiff_t>(*ua_size)) {
        return std::nullopt;
    }
    msg.user_agent.assign(reinterpret_cast<const char*>(ptr), static_cast<size_t>(*ua_size));
    ptr += *ua_size;

    if (end - ptr < 4) {
        return std::nullopt;
    }
    msg.start_height = ptr[0] | (static_cast<uint32_t>(ptr[1]) << 8) |
                       (static_cast<uint32_t>(ptr[2]) << 16) |
                       (static_cast<uint32_t>(ptr[3]) << 24);
    ptr += 4;

    msg.relay = (ptr < end) ? (*ptr != 0) : true;
    if (ptr < end) {
        ++ptr;
    }

    if (ptr != end) {
        return std::nullopt;
    }

    return msg;
}

std::vector<uint8_t> PingPongMessage::Serialize() const {
    std::vector<uint8_t> result;
    result.reserve(8);
    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>(nonce >> (8 * i)));
    }
    return result;
}

std::optional<PingPongMessage> PingPongMessage::Deserialize(const uint8_t* data, size_t len) {
    if (data == nullptr || len != 8) {
        return std::nullopt;
    }

    PingPongMessage msg;
    for (int i = 0; i < 8; ++i) {
        msg.nonce |= static_cast<uint64_t>(data[i]) << (8 * i);
    }
    return msg;
}

std::vector<uint8_t> InvVect::Serialize() const {
    std::vector<uint8_t> result;
    result.reserve(36);

    const uint32_t type_value = static_cast<uint32_t>(type);
    for (int i = 0; i < 4; ++i) {
        result.push_back(static_cast<uint8_t>(type_value >> (8 * i)));
    }
    result.insert(result.end(), hash.begin(), hash.end());

    return result;
}

std::optional<InvVect> InvVect::Deserialize(const uint8_t* data) {
    if (data == nullptr) {
        return std::nullopt;
    }

    InvVect inv;
    const uint32_t type_value =
        data[0] | (static_cast<uint32_t>(data[1]) << 8) |
        (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
    inv.type = static_cast<InvType>(type_value);
    std::copy(data + 4, data + 36, inv.hash.begin());
    return inv;
}

std::vector<uint8_t> InvMessage::Serialize() const {
    std::vector<uint8_t> result;

    WriteCompactSize(result, inventory.size());
    for (const auto& inv : inventory) {
        const auto bytes = inv.Serialize();
        result.insert(result.end(), bytes.begin(), bytes.end());
    }

    return result;
}

std::optional<InvMessage> InvMessage::Deserialize(const uint8_t* data, size_t len) {
    if (data == nullptr) {
        return std::nullopt;
    }

    InvMessage msg;
    const uint8_t* ptr = data;
    const uint8_t* end = data + len;

    auto count = ReadCompactSizeChecked(ptr, end);
    if (!count || *count > MAX_INV_SIZE || *count > (len / 36)) {
        return std::nullopt;
    }

    msg.inventory.reserve(static_cast<size_t>(*count));
    for (uint64_t i = 0; i < *count; ++i) {
        if (end - ptr < 36) {
            return std::nullopt;
        }
        auto inv = InvVect::Deserialize(ptr);
        if (!inv) {
            return std::nullopt;
        }
        msg.inventory.push_back(*inv);
        ptr += 36;
    }

    if (ptr != end) {
        return std::nullopt;
    }

    return msg;
}

std::vector<uint8_t> AddrMessage::Serialize() const {
    std::vector<uint8_t> result;
    WriteCompactSize(result, addresses.size());

    for (const auto& addr : addresses) {
        SerializeNetAddr(result, addr, true);
    }

    return result;
}

std::optional<AddrMessage> AddrMessage::Deserialize(const uint8_t* data, size_t len) {
    if (data == nullptr) {
        return std::nullopt;
    }

    AddrMessage msg;
    const uint8_t* ptr = data;
    const uint8_t* end = data + len;

    auto count = ReadCompactSizeChecked(ptr, end);
    if (!count || *count > MAX_ADDR_TO_SEND) {
        return std::nullopt;
    }

    msg.addresses.reserve(static_cast<size_t>(*count));
    for (uint64_t i = 0; i < *count; ++i) {
        NetAddr addr;
        if (!DeserializeNetAddr(ptr, end, addr, true)) {
            return std::nullopt;
        }
        msg.addresses.push_back(addr);
    }

    if (ptr != end) {
        return std::nullopt;
    }

    return msg;
}

std::vector<uint8_t> BlockMessage::Serialize() const {
    return block.Serialize();
}

std::optional<BlockMessage> BlockMessage::Deserialize(const uint8_t* data, size_t len) {
    auto block_opt = primitives::Block::Deserialize(data, len);
    if (!block_opt) {
        return std::nullopt;
    }

    if (block_opt->Serialize().size() != len) {
        return std::nullopt;
    }

    return BlockMessage(*block_opt);
}

std::vector<uint8_t> TxMessage::Serialize() const {
    return tx.Serialize();
}

std::optional<TxMessage> TxMessage::Deserialize(const uint8_t* data, size_t len) {
    auto tx_opt = primitives::Transaction::Deserialize(data, len);
    if (!tx_opt) {
        return std::nullopt;
    }

    if (tx_opt->Serialize().size() != len) {
        return std::nullopt;
    }

    return TxMessage(*tx_opt);
}

std::vector<uint8_t> GetHeadersMessage::Serialize() const {
    std::vector<uint8_t> result;

    for (int i = 0; i < 4; ++i) {
        result.push_back(static_cast<uint8_t>(version >> (8 * i)));
    }

    WriteCompactSize(result, block_locator_hashes.size());
    for (const auto& hash : block_locator_hashes) {
        result.insert(result.end(), hash.begin(), hash.end());
    }

    result.insert(result.end(), hash_stop.begin(), hash_stop.end());
    return result;
}

std::optional<GetHeadersMessage> GetHeadersMessage::Deserialize(const uint8_t* data, size_t len) {
    if (data == nullptr || len < 37) {
        return std::nullopt;
    }

    GetHeadersMessage msg;
    const uint8_t* ptr = data;
    const uint8_t* end = data + len;

    msg.version = ptr[0] | (static_cast<uint32_t>(ptr[1]) << 8) |
                  (static_cast<uint32_t>(ptr[2]) << 16) | (static_cast<uint32_t>(ptr[3]) << 24);
    ptr += 4;

    auto count = ReadCompactSizeChecked(ptr, end);
    if (!count || *count > MAX_HEADERS_COUNT) {
        return std::nullopt;
    }

    if (end - ptr < static_cast<std::ptrdiff_t>((*count * 32) + 32)) {
        return std::nullopt;
    }

    msg.block_locator_hashes.reserve(static_cast<size_t>(*count));
    for (uint64_t i = 0; i < *count; ++i) {
        std::array<uint8_t, 32> hash{};
        std::copy(ptr, ptr + 32, hash.begin());
        ptr += 32;
        msg.block_locator_hashes.push_back(hash);
    }

    std::copy(ptr, ptr + 32, msg.hash_stop.begin());
    ptr += 32;

    if (ptr != end) {
        return std::nullopt;
    }

    return msg;
}

std::vector<uint8_t> HeadersMessage::Serialize() const {
    std::vector<uint8_t> result;

    WriteCompactSize(result, headers.size());
    for (const auto& header : headers) {
        auto header_bytes = header.Serialize();
        result.insert(result.end(), header_bytes.begin(), header_bytes.end());
        WriteCompactSize(result, 0);  // txn count after each header in Bitcoin-style wire format
    }

    return result;
}

std::optional<HeadersMessage> HeadersMessage::Deserialize(const uint8_t* data, size_t len) {
    if (data == nullptr) {
        return std::nullopt;
    }

    HeadersMessage msg;
    const uint8_t* ptr = data;
    const uint8_t* end = data + len;

    auto count = ReadCompactSizeChecked(ptr, end);
    if (!count || *count > MAX_HEADERS_COUNT) {
        return std::nullopt;
    }

    msg.headers.reserve(static_cast<size_t>(*count));
    for (uint64_t i = 0; i < *count; ++i) {
        if (end - ptr < static_cast<std::ptrdiff_t>(kHeaderSize)) {
            return std::nullopt;
        }

        msg.headers.push_back(primitives::BlockHeader::Deserialize(ptr));
        ptr += kHeaderSize;

        auto tx_count = ReadCompactSizeChecked(ptr, end);
        if (!tx_count || *tx_count != 0) {
            return std::nullopt;
        }
    }

    if (ptr != end) {
        return std::nullopt;
    }

    return msg;
}

std::vector<uint8_t> RejectMessage::Serialize() const {
    std::vector<uint8_t> result;

    WriteCompactSize(result, message.size());
    result.insert(result.end(), message.begin(), message.end());

    result.push_back(ccode);

    WriteCompactSize(result, reason.size());
    result.insert(result.end(), reason.begin(), reason.end());

    result.insert(result.end(), data.begin(), data.end());

    return result;
}

std::optional<RejectMessage> RejectMessage::Deserialize(const uint8_t* data_ptr, size_t len) {
    if (data_ptr == nullptr) {
        return std::nullopt;
    }

    RejectMessage msg;
    const uint8_t* ptr = data_ptr;
    const uint8_t* end = data_ptr + len;

    auto message_size = ReadCompactSizeChecked(ptr, end);
    if (!message_size || *message_size > MAX_REJECT_MESSAGE_LENGTH ||
        end - ptr < static_cast<std::ptrdiff_t>(*message_size)) {
        return std::nullopt;
    }
    msg.message.assign(reinterpret_cast<const char*>(ptr), static_cast<size_t>(*message_size));
    ptr += *message_size;

    if (ptr >= end) {
        return std::nullopt;
    }
    msg.ccode = *ptr++;

    auto reason_size = ReadCompactSizeChecked(ptr, end);
    if (!reason_size || *reason_size > MAX_REJECT_MESSAGE_LENGTH ||
        end - ptr < static_cast<std::ptrdiff_t>(*reason_size)) {
        return std::nullopt;
    }
    msg.reason.assign(reinterpret_cast<const char*>(ptr), static_cast<size_t>(*reason_size));
    ptr += *reason_size;

    msg.data.assign(ptr, end);
    return msg;
}

uint32_t CalculateChecksum(const std::vector<uint8_t>& payload) {
    auto hash = crypto::SHA256d::Hash256d(payload);
    return hash[0] | (static_cast<uint32_t>(hash[1]) << 8) |
           (static_cast<uint32_t>(hash[2]) << 16) | (static_cast<uint32_t>(hash[3]) << 24);
}

std::vector<uint8_t> CreateNetworkMessage(uint32_t magic, const char* command,
                                          const std::vector<uint8_t>& payload) {
    MessageHeader header;
    header.magic = magic;
    std::memset(header.command, 0, sizeof(header.command));

    size_t command_length = std::strlen(command);
    if (command_length >= sizeof(header.command)) {
        command_length = sizeof(header.command) - 1;
    }
    std::memcpy(header.command, command, command_length);

    header.length = static_cast<uint32_t>(payload.size());
    header.checksum = CalculateChecksum(payload);

    auto result = header.Serialize();
    result.insert(result.end(), payload.begin(), payload.end());
    return result;
}

}  // namespace p2p
}  // namespace parthenon
