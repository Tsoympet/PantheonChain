// ParthenonChain - Hardware Wallet Unit Tests

#include "wallet/hardware/hardware_wallet.h"
#include <cassert>
#include <iostream>

using namespace parthenon::wallet::hardware;

void test_derivation_path_parsing() {
    // Test valid paths
    auto path1 = DerivationPath::Parse("m/44'/0'/0'/0/0");
    assert(path1.has_value());
    assert(path1->path.size() == 5);
    assert((path1->path[0] & 0x80000000) != 0);  // Hardened
    assert((path1->path[1] & 0x80000000) != 0);  // Hardened
    assert((path1->path[2] & 0x80000000) != 0);  // Hardened
    assert((path1->path[3] & 0x80000000) == 0);  // Not hardened
    assert((path1->path[4] & 0x80000000) == 0);  // Not hardened
    
    // Test path to string conversion
    std::string path_str = path1->ToString();
    assert(path_str == "m/44'/0'/0'/0/0");
    
    // Test invalid paths
    auto invalid1 = DerivationPath::Parse("invalid");
    assert(!invalid1.has_value());
    
    auto invalid2 = DerivationPath::Parse("");
    assert(!invalid2.has_value());
    
    std::cout << "✓ Derivation path parsing tests passed\n";
}

void test_hardware_wallet_manager() {
    HardwareWalletManager manager;
    
    // Enumerate devices (will be empty in test environment)
    auto devices = manager.EnumerateDevices();
    
    // Try to connect (will fail if no devices)
    auto wallet = manager.ConnectFirstDevice();
    
    // In test environment with no physical devices, this is expected
    if (!wallet) {
        std::cout << "✓ Hardware wallet manager created (no devices detected)\n";
    } else {
        std::cout << "✓ Hardware wallet manager created and connected to device\n";
    }
}

void test_derivation_path_construction() {
    // Test constructor with vector
    std::vector<uint32_t> path_data = {
        44 | 0x80000000,  // 44'
        0 | 0x80000000,   // 0'
        0 | 0x80000000,   // 0'
        0,                // 0
        0                 // 0
    };
    
    DerivationPath path(path_data);
    assert(path.path.size() == 5);
    assert(path.ToString() == "m/44'/0'/0'/0/0");
    
    std::cout << "✓ Derivation path construction tests passed\n";
}

int main() {
    std::cout << "Running hardware wallet tests...\n\n";
    
    test_derivation_path_parsing();
    test_derivation_path_construction();
    test_hardware_wallet_manager();
    
    std::cout << "\nAll hardware wallet tests passed!\n";
    return 0;
}
