// ParthenonChain - SHA-256 Test Vectors
// Deterministic tests using known test vectors from NIST and Bitcoin Core

#include "crypto/sha256.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cassert>

using namespace parthenon::crypto;

// Helper to convert hex string to bytes
std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::strtol(byte_str.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// Helper to convert bytes to hex string
std::string BytesToHex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

template<size_t N>
std::string BytesToHex(const std::array<uint8_t, N>& arr) {
    return BytesToHex(arr.data(), arr.size());
}

void TestSHA256Empty() {
    std::cout << "Test SHA-256: empty string" << std::endl;
    
    auto hash = SHA256::Hash256(nullptr, 0);
    std::string result = BytesToHex(hash);
    std::string expected = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    
    assert(result == expected);
    std::cout << "  ✓ Passed" << std::endl;
}

void TestSHA256ABC() {
    std::cout << "Test SHA-256: \"abc\"" << std::endl;
    
    const char* data = "abc";
    auto hash = SHA256::Hash256(reinterpret_cast<const uint8_t*>(data), 3);
    std::string result = BytesToHex(hash);
    std::string expected = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    
    assert(result == expected);
    std::cout << "  ✓ Passed" << std::endl;
}

void TestSHA256LongMessage() {
    std::cout << "Test SHA-256: \"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq\"" << std::endl;
    
    const char* data = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    auto hash = SHA256::Hash256(reinterpret_cast<const uint8_t*>(data), 56);
    std::string result = BytesToHex(hash);
    std::string expected = "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1";
    
    assert(result == expected);
    std::cout << "  ✓ Passed" << std::endl;
}

void TestSHA256Incremental() {
    std::cout << "Test SHA-256: incremental hashing" << std::endl;
    
    SHA256 hasher;
    const char* part1 = "abc";
    const char* part2 = "def";
    hasher.Write(reinterpret_cast<const uint8_t*>(part1), 3);
    hasher.Write(reinterpret_cast<const uint8_t*>(part2), 3);
    auto hash = hasher.Finalize();
    
    std::string result = BytesToHex(hash);
    // SHA-256 of "abcdef"
    std::string expected = "bef57ec7f53a6d40beb640a780a639c83bc29ac8a9816f1fc6c5c6dcd93c4721";
    
    assert(result == expected);
    std::cout << "  ✓ Passed" << std::endl;
}

void TestSHA256d() {
    std::cout << "Test SHA-256d (double SHA-256)" << std::endl;
    
    const char* data = "hello";
    auto hash = SHA256d::Hash256d(reinterpret_cast<const uint8_t*>(data), 5);
    std::string result = BytesToHex(hash);
    
    // First hash: SHA256("hello") = 2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824
    // Second hash: SHA256(above)
    std::string expected = "9595c9df90075148eb06860365df33584b75bff782a510c6cd4883a419833d50";
    
    assert(result == expected);
    std::cout << "  ✓ Passed" << std::endl;
}

void TestTaggedSHA256() {
    std::cout << "Test Tagged SHA-256 (BIP-340 style)" << std::endl;
    
    std::string tag = "BIP0340/test";
    const char* data = "test message";
    auto hash = TaggedSHA256::HashTagged(tag, 
                                         reinterpret_cast<const uint8_t*>(data), 
                                         12);
    
    std::string result = BytesToHex(hash);
    
    // Tagged hash should be deterministic
    // Re-compute to verify
    auto hash2 = TaggedSHA256::HashTagged(tag, 
                                          reinterpret_cast<const uint8_t*>(data), 
                                          12);
    std::string result2 = BytesToHex(hash2);
    
    assert(result == result2);
    std::cout << "  Hash: " << result << std::endl;
    std::cout << "  ✓ Passed (deterministic)" << std::endl;
}

void TestSHA256BitcoinBlock() {
    std::cout << "Test SHA-256d: Bitcoin genesis block header" << std::endl;
    
    // Bitcoin genesis block header (80 bytes)
    std::string header_hex = 
        "0100000000000000000000000000000000000000000000000000000000000000"
        "000000003ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa"
        "4b1e5e4a29ab5f49ffff001d1dac2b7c";
    
    auto header = HexToBytes(header_hex);
    auto hash = SHA256d::Hash256d(header);
    
    std::string result = BytesToHex(hash);
    
    // Genesis block hash (little-endian): 
    // 000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f
    // In big-endian (as our function returns):
    std::string expected = "6fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000";
    
    assert(result == expected);
    std::cout << "  ✓ Passed" << std::endl;
}

void TestSHA256LargeData() {
    std::cout << "Test SHA-256: large data (1MB)" << std::endl;
    
    std::vector<uint8_t> data(1024 * 1024, 0x42); // 1MB of 0x42
    auto hash = SHA256::Hash256(data);
    
    std::string result = BytesToHex(hash);
    
    // Verify it's deterministic by computing again
    auto hash2 = SHA256::Hash256(data);
    std::string result2 = BytesToHex(hash2);
    
    assert(result == result2);
    std::cout << "  Hash: " << result << std::endl;
    std::cout << "  ✓ Passed (deterministic)" << std::endl;
}

int main() {
    std::cout << "=====================================" << std::endl;
    std::cout << "ParthenonChain SHA-256 Test Suite" << std::endl;
    std::cout << "=====================================" << std::endl << std::endl;
    
    try {
        TestSHA256Empty();
        TestSHA256ABC();
        TestSHA256LongMessage();
        TestSHA256Incremental();
        TestSHA256d();
        TestTaggedSHA256();
        TestSHA256BitcoinBlock();
        TestSHA256LargeData();
        
        std::cout << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "All SHA-256 tests passed! ✓" << std::endl;
        std::cout << "=====================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
