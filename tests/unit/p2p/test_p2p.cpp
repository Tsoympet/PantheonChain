// ParthenonChain - P2P Protocol Tests
// Test network protocol and message serialization

#include "p2p/message.h"
#include "p2p/protocol.h"

#include <cassert>
#include <cstring>
#include <iostream>

using namespace parthenon::p2p;

namespace {
std::vector<uint8_t> EncodeCompactSize(uint64_t size) {
    std::vector<uint8_t> result;
    if (size < 253) {
        result.push_back(static_cast<uint8_t>(size));
    } else if (size <= 0xFFFF) {
        result.push_back(253);
        result.push_back(static_cast<uint8_t>(size));
        result.push_back(static_cast<uint8_t>(size >> 8));
    } else if (size <= 0xFFFFFFFF) {
        result.push_back(254);
        result.push_back(static_cast<uint8_t>(size));
        result.push_back(static_cast<uint8_t>(size >> 8));
        result.push_back(static_cast<uint8_t>(size >> 16));
        result.push_back(static_cast<uint8_t>(size >> 24));
    } else {
        result.push_back(255);
        for (int i = 0; i < 8; i++) {
            result.push_back(static_cast<uint8_t>(size >> (8 * i)));
        }
    }
    return result;
}

parthenon::primitives::Transaction MakeTestTransaction() {
    parthenon::primitives::Transaction tx;
    tx.version = 2;
    tx.locktime = 0;

    parthenon::primitives::TxInput input;
    std::array<uint8_t, 32> txid{};
    txid[0] = 0x42;
    input.prevout = parthenon::primitives::OutPoint(txid, 1);
    input.signature_script = {0x01, 0x02, 0x03};
    input.sequence = 0xFFFFFFFE;
    tx.inputs.push_back(input);

    parthenon::primitives::TxOutput output;
    output.value = parthenon::primitives::AssetAmount(parthenon::primitives::AssetID::TALANTON, 500);
    output.pubkey_script = std::vector<uint8_t>(32, 0x11);
    tx.outputs.push_back(output);

    return tx;
}

parthenon::primitives::Block MakeTestBlock() {
    parthenon::primitives::Block block;
    block.header.version = 3;
    block.header.timestamp = 1234567890;
    block.header.bits = 0x1d00ffff;
    block.header.nonce = 42;
    block.transactions.push_back(MakeTestTransaction());
    block.header.merkle_root = block.CalculateMerkleRoot();
    return block;
}
}  // namespace

void TestNetAddrValidation() {
    std::cout << "Test: Network address validation" << std::endl;

    NetAddr addr;

    // Test IPv4 mapping
    // IPv4-mapped IPv6: ::ffff:192.168.1.1
    std::memset(addr.ip, 0, 10);
    addr.ip[10] = 0xFF;
    addr.ip[11] = 0xFF;
    addr.ip[12] = 192;
    addr.ip[13] = 168;
    addr.ip[14] = 1;
    addr.ip[15] = 1;

    assert(addr.IsIPv4());
    assert(!addr.IsRoutable());  // 192.168.x.x is private

    // Test public IP
    addr.ip[12] = 8;
    addr.ip[13] = 8;
    addr.ip[14] = 8;
    addr.ip[15] = 8;

    assert(addr.IsRoutable());  // 8.8.8.8 is public

    std::cout << "  ✓ Passed (address validation)" << std::endl;
}

void TestMessageHeaderSerialization() {
    std::cout << "Test: Message header serialization" << std::endl;

    MessageHeader header;
    header.magic = NetworkMagic::MAINNET;
    constexpr char kVersionCommand[] = "version";
    std::memset(header.command, 0, sizeof(header.command));
    std::memcpy(header.command, kVersionCommand, sizeof(kVersionCommand) - 1);
    header.length = 100;
    header.checksum = 0x12345678;

    // Serialize
    auto bytes = header.Serialize();
    assert(bytes.size() == 24);

    // Deserialize
    auto deserialized = MessageHeader::Deserialize(bytes.data());
    assert(deserialized.has_value());
    assert(deserialized->magic == header.magic);
    assert(std::strcmp(deserialized->command, "version") == 0);
    assert(deserialized->length == 100);
    assert(deserialized->checksum == 0x12345678);

    // Validate
    assert(deserialized->IsValid(NetworkMagic::MAINNET));
    assert(!deserialized->IsValid(NetworkMagic::TESTNET));

    std::cout << "  ✓ Passed (header serialization)" << std::endl;
}

void TestPingPongMessage() {
    std::cout << "Test: Ping/Pong message" << std::endl;

    PingPongMessage ping(0x123456789ABCDEF0ULL);

    // Serialize
    auto bytes = ping.Serialize();
    assert(bytes.size() == 8);

    // Deserialize
    auto deserialized = PingPongMessage::Deserialize(bytes.data(), bytes.size());
    assert(deserialized.has_value());
    assert(deserialized->nonce == 0x123456789ABCDEF0ULL);

    std::cout << "  ✓ Passed (ping/pong)" << std::endl;
}

void TestInvMessage() {
    std::cout << "Test: Inventory message" << std::endl;

    InvMessage inv;

    // Add some inventory items
    std::array<uint8_t, 32> hash1{};
    hash1[0] = 1;
    inv.inventory.push_back(InvVect(InvType::MSG_TX, hash1));

    std::array<uint8_t, 32> hash2{};
    hash2[0] = 2;
    inv.inventory.push_back(InvVect(InvType::MSG_BLOCK, hash2));

    // Serialize
    auto bytes = inv.Serialize();

    // Deserialize
    auto deserialized = InvMessage::Deserialize(bytes.data(), bytes.size());
    assert(deserialized.has_value());
    assert(deserialized->inventory.size() == 2);
    assert(deserialized->inventory[0].type == InvType::MSG_TX);
    assert(deserialized->inventory[1].type == InvType::MSG_BLOCK);
    assert(deserialized->inventory[0].hash[0] == 1);
    assert(deserialized->inventory[1].hash[0] == 2);

    std::cout << "  ✓ Passed (inventory message)" << std::endl;
}

void TestVersionMessage() {
    std::cout << "Test: Version message" << std::endl;

    VersionMessage ver;
    ver.version = PROTOCOL_VERSION;
    ver.services = static_cast<uint64_t>(ServiceFlags::NODE_NETWORK);
    ver.timestamp = 1234567890;
    ver.nonce = 0xABCD1234;
    ver.user_agent = "/ParthenonChain:0.1.0/";
    ver.start_height = 12345;
    ver.relay = true;

    // Serialize
    auto bytes = ver.Serialize();

    // Deserialize
    auto deserialized = VersionMessage::Deserialize(bytes.data(), bytes.size());
    assert(deserialized.has_value());
    assert(deserialized->version == PROTOCOL_VERSION);
    assert(deserialized->nonce == 0xABCD1234);
    assert(deserialized->user_agent == "/ParthenonChain:0.1.0/");
    assert(deserialized->start_height == 12345);
    assert(deserialized->relay == true);

    std::cout << "  ✓ Passed (version message)" << std::endl;
}

void TestAddrMessage() {
    std::cout << "Test: Addr message" << std::endl;

    AddrMessage addr_msg;
    NetAddr addr;
    addr.time = 0x01020304;
    addr.services = 0x0102030405060708ULL;
    std::memset(addr.ip, 0, 16);
    addr.ip[10] = 0xFF;
    addr.ip[11] = 0xFF;
    addr.ip[12] = 1;
    addr.ip[13] = 2;
    addr.ip[14] = 3;
    addr.ip[15] = 4;
    addr.port = 8333;
    addr_msg.addresses.push_back(addr);

    auto bytes = addr_msg.Serialize();

    std::vector<uint8_t> expected = {
        0x01,
        0x04, 0x03, 0x02, 0x01,
        0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x01, 0x02,
        0x03, 0x04,
        0x20, 0x8D};
    assert(bytes == expected);

    auto deserialized = AddrMessage::Deserialize(bytes.data(), bytes.size());
    assert(deserialized.has_value());
    assert(deserialized->addresses.size() == 1);
    assert(deserialized->addresses[0].port == 8333);
    assert(deserialized->addresses[0].time == 0x01020304);

    auto truncated = bytes;
    truncated.pop_back();
    assert(!AddrMessage::Deserialize(truncated.data(), truncated.size()));

    auto overflow = EncodeCompactSize(MAX_ADDR_TO_SEND + 1);
    assert(!AddrMessage::Deserialize(overflow.data(), overflow.size()));

    std::cout << "  ✓ Passed (addr message)" << std::endl;
}

void TestBlockAndTxMessages() {
    std::cout << "Test: Block/Tx messages" << std::endl;

    TxMessage tx_msg(MakeTestTransaction());
    auto tx_bytes = tx_msg.Serialize();
    auto tx_deserialized = TxMessage::Deserialize(tx_bytes.data(), tx_bytes.size());
    assert(tx_deserialized.has_value());
    assert(tx_deserialized->tx.Serialize() == tx_msg.tx.Serialize());

    auto tx_truncated = tx_bytes;
    tx_truncated.pop_back();
    assert(!TxMessage::Deserialize(tx_truncated.data(), tx_truncated.size()));

    BlockMessage block_msg(MakeTestBlock());
    auto block_bytes = block_msg.Serialize();
    auto block_deserialized = BlockMessage::Deserialize(block_bytes.data(), block_bytes.size());
    assert(block_deserialized.has_value());
    assert(block_deserialized->block.Serialize() == block_msg.block.Serialize());

    auto block_truncated = block_bytes;
    block_truncated.pop_back();
    assert(!BlockMessage::Deserialize(block_truncated.data(), block_truncated.size()));

    std::cout << "  ✓ Passed (block/tx messages)" << std::endl;
}

void TestHeadersMessages() {
    std::cout << "Test: GetHeaders/Headers messages" << std::endl;

    GetHeadersMessage get_headers;
    std::array<uint8_t, 32> locator{};
    locator.fill(0x11);
    get_headers.block_locator_hashes.push_back(locator);
    get_headers.hash_stop.fill(0x00);

    auto get_headers_bytes = get_headers.Serialize();
    std::vector<uint8_t> expected = {0x71, 0x11, 0x01, 0x00, 0x01};
    expected.insert(expected.end(), locator.begin(), locator.end());
    expected.insert(expected.end(), get_headers.hash_stop.begin(), get_headers.hash_stop.end());
    assert(get_headers_bytes == expected);

    auto get_headers_deserialized =
        GetHeadersMessage::Deserialize(get_headers_bytes.data(), get_headers_bytes.size());
    assert(get_headers_deserialized.has_value());
    assert(get_headers_deserialized->block_locator_hashes.size() == 1);

    auto get_headers_truncated = get_headers_bytes;
    get_headers_truncated.pop_back();
    assert(!GetHeadersMessage::Deserialize(get_headers_truncated.data(), get_headers_truncated.size()));

    auto overflow = EncodeCompactSize(MAX_HEADERS_COUNT + 1);
    overflow.insert(overflow.end(), 32, 0x00);
    assert(!GetHeadersMessage::Deserialize(overflow.data(), overflow.size()));

    HeadersMessage headers_msg;
    parthenon::primitives::BlockHeader header;
    header.version = 5;
    header.timestamp = 42;
    headers_msg.headers.push_back(header);

    auto headers_bytes = headers_msg.Serialize();
    auto headers_deserialized = HeadersMessage::Deserialize(headers_bytes.data(), headers_bytes.size());
    assert(headers_deserialized.has_value());
    assert(headers_deserialized->headers.size() == 1);
    assert(headers_deserialized->headers[0].version == 5);

    auto headers_truncated = headers_bytes;
    headers_truncated.pop_back();
    assert(!HeadersMessage::Deserialize(headers_truncated.data(), headers_truncated.size()));

    std::cout << "  ✓ Passed (headers messages)" << std::endl;
}

void TestNetworkMessageCreation() {
    std::cout << "Test: Network message creation" << std::endl;

    PingPongMessage ping(0x123456);
    auto payload = ping.Serialize();

    auto message = CreateNetworkMessage(NetworkMagic::MAINNET, "ping", payload);

    // Should have header (24 bytes) + payload (8 bytes)
    assert(message.size() == 32);

    // Verify header
    auto header = MessageHeader::Deserialize(message.data());
    assert(header.has_value());
    assert(header->magic == NetworkMagic::MAINNET);
    assert(std::strcmp(header->command, "ping") == 0);
    assert(header->length == 8);

    std::cout << "  ✓ Passed (message creation)" << std::endl;
}

int main() {
    std::cout << "=== P2P Protocol Tests ===" << std::endl;

    TestNetAddrValidation();
    TestMessageHeaderSerialization();
    TestPingPongMessage();
    TestInvMessage();
    TestVersionMessage();
    TestAddrMessage();
    TestBlockAndTxMessages();
    TestHeadersMessages();
    TestNetworkMessageCreation();

    std::cout << "\n✓ All P2P tests passed!" << std::endl;
    return 0;
}
