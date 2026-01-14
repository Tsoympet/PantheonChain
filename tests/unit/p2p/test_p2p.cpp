// ParthenonChain - P2P Protocol Tests
// Test network protocol and message serialization

#include "p2p/message.h"
#include "p2p/protocol.h"

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
    std::strncpy(header.command, "version", 12);
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

int main() {
    std::cout << "=== P2P Protocol Tests ===" << std::endl;

    TestNetAddrValidation();
    TestMessageHeaderSerialization();
    TestPingPongMessage();
    TestInvMessage();
    TestVersionMessage();
    TestNetworkMessageCreation();

    std::cout << "\n✓ All P2P tests passed!" << std::endl;
    return 0;
}
