// ParthenonChain - P2P Protocol Tests
// Test network protocol and message serialization

#include "p2p/message.h"
#include "p2p/protocol.h"
#include "primitives/block.h"
#include "primitives/transaction.h"

#include <cassert>
#include <cstring>
#include <iostream>

using namespace parthenon::p2p;

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

void TestAddrMessage() {
    std::cout << "Test: Addr message" << std::endl;

    AddrMessage addr_msg;
    NetAddr addr1;
    addr1.time = 123;
    addr1.services = static_cast<uint64_t>(ServiceFlags::NODE_NETWORK);
    std::memset(addr1.ip, 0, sizeof(addr1.ip));
    addr1.ip[10] = 0xFF;
    addr1.ip[11] = 0xFF;
    addr1.ip[12] = 1;
    addr1.ip[13] = 2;
    addr1.ip[14] = 3;
    addr1.ip[15] = 4;
    addr1.port = 8333;
    addr_msg.addresses.push_back(addr1);

    auto bytes = addr_msg.Serialize();
    auto parsed = AddrMessage::Deserialize(bytes.data(), bytes.size());
    assert(parsed.has_value());
    assert(parsed->addresses.size() == 1);
    assert(parsed->addresses[0].time == 123);
    assert(parsed->addresses[0].port == 8333);
    assert(parsed->addresses[0].ip[15] == 4);

    std::cout << "  ✓ Passed (addr message)" << std::endl;
}

void TestGetHeadersMessage() {
    std::cout << "Test: GetHeaders message" << std::endl;

    GetHeadersMessage get_headers;
    get_headers.version = PROTOCOL_VERSION;

    std::array<uint8_t, 32> locator1{};
    locator1[0] = 0xAA;
    std::array<uint8_t, 32> locator2{};
    locator2[1] = 0xBB;
    get_headers.block_locator_hashes.push_back(locator1);
    get_headers.block_locator_hashes.push_back(locator2);
    get_headers.hash_stop[31] = 0xCC;

    auto bytes = get_headers.Serialize();
    auto parsed = GetHeadersMessage::Deserialize(bytes.data(), bytes.size());
    assert(parsed.has_value());
    assert(parsed->version == PROTOCOL_VERSION);
    assert(parsed->block_locator_hashes.size() == 2);
    assert(parsed->block_locator_hashes[0][0] == 0xAA);
    assert(parsed->block_locator_hashes[1][1] == 0xBB);
    assert(parsed->hash_stop[31] == 0xCC);

    std::cout << "  ✓ Passed (getheaders message)" << std::endl;
}

void TestHeadersMessage() {
    std::cout << "Test: Headers message" << std::endl;

    HeadersMessage headers;
    parthenon::primitives::BlockHeader h1;
    h1.version = 2;
    h1.timestamp = 1000;
    h1.bits = 0x1d00ffff;
    h1.nonce = 42;
    headers.headers.push_back(h1);

    auto bytes = headers.Serialize();
    auto parsed = HeadersMessage::Deserialize(bytes.data(), bytes.size());
    assert(parsed.has_value());
    assert(parsed->headers.size() == 1);
    assert(parsed->headers[0].version == 2);
    assert(parsed->headers[0].timestamp == 1000);
    assert(parsed->headers[0].nonce == 42);

    std::cout << "  ✓ Passed (headers message)" << std::endl;
}

void TestTxAndBlockMessages() {
    std::cout << "Test: Tx and Block messages" << std::endl;

    parthenon::primitives::Transaction tx;
    tx.version = 1;
    tx.locktime = 0;
    parthenon::primitives::TxInput in;
    in.prevout.vout = parthenon::primitives::COINBASE_VOUT_INDEX;
    in.signature_script = {0x01};
    tx.inputs.push_back(in);
    tx.outputs.emplace_back(parthenon::primitives::AssetID::TALANTON, 50,
                            std::vector<uint8_t>{0x51});

    TxMessage tx_msg(tx);
    auto tx_bytes = tx_msg.Serialize();
    auto tx_parsed = TxMessage::Deserialize(tx_bytes.data(), tx_bytes.size());
    assert(tx_parsed.has_value());
    assert(tx_parsed->tx.version == tx.version);
    assert(tx_parsed->tx.outputs.size() == 1);

    parthenon::primitives::Block block;
    block.transactions.push_back(tx);
    block.header.merkle_root = block.CalculateMerkleRoot();

    BlockMessage block_msg(block);
    auto block_bytes = block_msg.Serialize();
    auto block_parsed = BlockMessage::Deserialize(block_bytes.data(), block_bytes.size());
    assert(block_parsed.has_value());
    assert(block_parsed->block.transactions.size() == 1);
    assert(block_parsed->block.header.merkle_root == block.header.merkle_root);

    std::cout << "  ✓ Passed (tx/block messages)" << std::endl;
}

int main() {
    std::cout << "=== P2P Protocol Tests ===" << std::endl;

    TestNetAddrValidation();
    TestMessageHeaderSerialization();
    TestPingPongMessage();
    TestInvMessage();
    TestVersionMessage();
    TestNetworkMessageCreation();
    TestAddrMessage();
    TestGetHeadersMessage();
    TestHeadersMessage();
    TestTxAndBlockMessages();

    std::cout << "\n✓ All P2P tests passed!" << std::endl;
    return 0;
}
