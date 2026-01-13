// ParthenonChain - Genesis Block Builder
// Tool for generating genesis blocks for different networks

#include <iostream>
#include <fstream>
#include <ctime>
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "consensus/issuance.h"

using namespace parthenon;

/**
 * Parse hex string to bytes
 * 
 * Converts a hexadecimal string to a vector of bytes.
 * Supports strings with or without "0x" prefix.
 * 
 * @param hex_str Hexadecimal string to parse (e.g., "0x1234abcd" or "1234abcd")
 * @return Vector of bytes representing the parsed hex data
 *         Empty vector if parsing fails (invalid characters or odd length)
 * 
 * @note The function expects an even-length hex string (after removing prefix)
 * @note Valid hex characters are 0-9, a-f, A-F
 * @note Errors are logged to stderr and result in empty vector return
 */
std::vector<uint8_t> ParseHexString(const std::string& hex_str) {
    std::vector<uint8_t> result;
    
    // Remove "0x" prefix if present
    std::string hex = hex_str;
    if (hex.size() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
        hex = hex.substr(2);
    }
    
    // Hex string must have even length
    if (hex.size() % 2 != 0) {
        std::cerr << "Invalid hex string (odd length): " << hex_str << std::endl;
        return result;
    }
    
    // Parse hex pairs
    for (size_t i = 0; i < hex.size(); i += 2) {
        char high = hex[i];
        char low = hex[i + 1];
        
        // Convert hex chars to nibbles
        auto hex_char_to_nibble = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        
        int high_nibble = hex_char_to_nibble(high);
        int low_nibble = hex_char_to_nibble(low);
        
        if (high_nibble < 0 || low_nibble < 0) {
            std::cerr << "Invalid hex character in: " << hex_str << std::endl;
            return std::vector<uint8_t>();  // Return empty on error
        }
        
        result.push_back(static_cast<uint8_t>((high_nibble << 4) | low_nibble));
    }
    
    return result;
}

/**
 * Genesis configuration
 */
struct GenesisConfig {
    std::string network_name;  // "mainnet", "testnet", "regtest"
    uint32_t timestamp;
    uint32_t difficulty_bits;
    std::vector<std::string> premine_addresses;  // Initial fund recipients
};

/**
 * Create genesis block
 */
primitives::Block CreateGenesisBlock(const GenesisConfig& config) {
    primitives::Block genesis;
    
    // Set genesis header
    genesis.header.version = 1;
    genesis.header.prev_block_hash = std::array<uint8_t, 32>{};  // All zeros
    genesis.header.timestamp = config.timestamp;
    genesis.header.bits = config.difficulty_bits;
    genesis.header.nonce = 0;  // Will be mined
    
    // Create coinbase transaction
    primitives::Transaction coinbase;
    coinbase.version = 1;
    coinbase.locktime = 0;
    
    // Coinbase input (no previous output)
    primitives::TxInput coinbase_input;
    coinbase_input.prevout.txid = std::array<uint8_t, 32>{};
    coinbase_input.prevout.vout = 0xFFFFFFFF;
    
    // Genesis message in signature script
    std::string genesis_message = "ParthenonChain - " + config.network_name;
    coinbase_input.signature_script = std::vector<uint8_t>(
        genesis_message.begin(), genesis_message.end()
    );
    coinbase_input.sequence = 0xFFFFFFFF;
    coinbase.inputs.push_back(coinbase_input);
    
    // Coinbase outputs (premine to specified addresses)
    if (!config.premine_addresses.empty()) {
        // Distribute initial supply (using ULL suffix to avoid overflow)
        uint64_t tal_per_address = 1000000ULL * 100000000ULL;  // 1M TALANTON each
        uint64_t dra_per_address = 2000000ULL * 100000000ULL;  // 2M DRACHMA each
        uint64_t obl_per_address = 3000000ULL * 100000000ULL;  // 3M OBOLOS each
        
        for (const auto& addr_hex : config.premine_addresses) {
            // Parse hex address to pubkey
            std::vector<uint8_t> pubkey_script = ParseHexString(addr_hex);
            
            // Validate parsed address (should be 32 bytes for x-only Schnorr pubkey)
            if (pubkey_script.size() != 32) {
                std::cerr << "Warning: Invalid address length (" << pubkey_script.size() 
                         << " bytes, expected 32): " << addr_hex << std::endl;
                std::cerr << "Using dummy 32-byte address instead" << std::endl;
                pubkey_script = std::vector<uint8_t>(32, 0);  // Fallback to dummy
            }
            
            primitives::TxOutput tal_output;
            tal_output.value = primitives::AssetAmount(primitives::AssetID::TALANTON, tal_per_address);
            tal_output.pubkey_script = pubkey_script;
            coinbase.outputs.push_back(tal_output);
            
            primitives::TxOutput dra_output;
            dra_output.value = primitives::AssetAmount(primitives::AssetID::DRACHMA, dra_per_address);
            dra_output.pubkey_script = pubkey_script;
            coinbase.outputs.push_back(dra_output);
            
            primitives::TxOutput obl_output;
            obl_output.value = primitives::AssetAmount(primitives::AssetID::OBOLOS, obl_per_address);
            obl_output.pubkey_script = pubkey_script;
            coinbase.outputs.push_back(obl_output);
        }
    }
    
    genesis.transactions.push_back(coinbase);
    
    // Calculate merkle root
    genesis.header.merkle_root = genesis.CalculateMerkleRoot();
    
    return genesis;
}

/**
 * Mine genesis block (find valid nonce)
 */
void MineGenesisBlock(primitives::Block& genesis) {
    std::cout << "Mining genesis block..." << std::endl;
    
    for (uint64_t nonce = 0; nonce < 0x100000000; nonce++) {
        genesis.header.nonce = static_cast<uint32_t>(nonce);
        
        if (genesis.header.MeetsDifficultyTarget()) {
            std::cout << "Genesis block mined! Nonce: " << nonce << std::endl;
            return;
        }
        
        if (nonce % 100000 == 0) {
            std::cout << "  Tried " << nonce << " nonces..." << std::endl;
        }
    }
    
    std::cout << "Failed to mine genesis block in 2^32 attempts!" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "ParthenonChain Genesis Block Builder" << std::endl;
    std::cout << "====================================" << std::endl << std::endl;
    
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <network>" << std::endl;
        std::cout << "  network: mainnet, testnet, or regtest" << std::endl;
        return 1;
    }
    
    std::string network = argv[1];
    
    GenesisConfig config;
    config.network_name = network;
    config.timestamp = static_cast<uint32_t>(std::time(nullptr));
    
    if (network == "mainnet") {
        config.difficulty_bits = 0x1d00ffff;  // Bitcoin's initial difficulty
        // No premine for mainnet
    } else if (network == "testnet") {
        config.difficulty_bits = 0x1d00ffff;
        // Small premine for testing
        config.premine_addresses = {"testnet_dev_address"};
    } else if (network == "regtest") {
        config.difficulty_bits = 0x207fffff;  // Very low difficulty
        config.premine_addresses = {"regtest_dev_address"};
    } else {
        std::cerr << "Unknown network: " << network << std::endl;
        return 1;
    }
    
    std::cout << "Network: " << config.network_name << std::endl;
    std::cout << "Timestamp: " << config.timestamp << std::endl;
    std::cout << "Difficulty: 0x" << std::hex << config.difficulty_bits << std::dec << std::endl;
    std::cout << std::endl;
    
    // Create genesis block
    auto genesis = CreateGenesisBlock(config);
    
    // Mine it
    MineGenesisBlock(genesis);
    
    // Serialize and save
    auto serialized = genesis.Serialize();
    auto hash = genesis.GetHash();
    
    std::cout << std::endl;
    std::cout << "Genesis Block Created!" << std::endl;
    std::cout << "Hash: ";
    for (uint8_t byte : hash) {
        printf("%02x", byte);
    }
    std::cout << std::endl;
    
    // Save to file
    std::string filename = "genesis_" + network + ".dat";
    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<const char*>(serialized.data()), serialized.size());
    file.close();
    
    std::cout << "Saved to: " << filename << std::endl;
    
    return 0;
}
