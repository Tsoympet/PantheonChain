// ParthenonChain - Chain Parameters Tool
// Utility for managing network parameters

#include <iostream>
#include <fstream>
#include <string>

/**
 * Network parameters
 */
struct ChainParams {
    std::string network_name;
    uint32_t default_port;
    uint32_t rpc_port;
    uint32_t target_block_time;  // seconds
    uint32_t difficulty_adjustment_interval;  // blocks
    uint64_t max_supply_talanton;
    uint64_t max_supply_drachma;
    uint64_t max_supply_obolos;
};

const ChainParams MAINNET_PARAMS = {
    "mainnet",
    8333,  // P2P port
    8332,  // RPC port
    600,   // 10 minutes
    2016,  // ~2 weeks
    21000000ULL * 100000000ULL,  // 21M TALANTON
    41000000ULL * 100000000ULL,  // 41M DRACHMA
    61000000ULL * 100000000ULL   // 61M OBOLOS
};

const ChainParams TESTNET_PARAMS = {
    "testnet",
    18333,  // P2P port
    18332,  // RPC port
    600,    // 10 minutes
    2016,   // ~2 weeks
    21000000ULL * 100000000ULL,
    41000000ULL * 100000000ULL,
    61000000ULL * 100000000ULL
};

const ChainParams REGTEST_PARAMS = {
    "regtest",
    18444,  // P2P port
    18443,  // RPC port
    1,      // 1 second (instant blocks)
    144,    // Faster adjustment
    21000000ULL * 100000000ULL,
    41000000ULL * 100000000ULL,
    61000000ULL * 100000000ULL
};

void PrintParams(const ChainParams& params) {
    std::cout << "Network: " << params.network_name << std::endl;
    std::cout << "P2P Port: " << params.default_port << std::endl;
    std::cout << "RPC Port: " << params.rpc_port << std::endl;
    std::cout << "Target Block Time: " << params.target_block_time << " seconds" << std::endl;
    std::cout << "Difficulty Adjustment: every " << params.difficulty_adjustment_interval << " blocks" << std::endl;
    std::cout << "Max Supply TALANTON: " << (params.max_supply_talanton / 100000000) << std::endl;
    std::cout << "Max Supply DRACHMA: " << (params.max_supply_drachma / 100000000) << std::endl;
    std::cout << "Max Supply OBOLOS: " << (params.max_supply_obolos / 100000000) << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "ParthenonChain - Chain Parameters" << std::endl;
    std::cout << "==================================" << std::endl << std::endl;
    
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <network>" << std::endl;
        std::cout << "  network: mainnet, testnet, or regtest" << std::endl;
        return 1;
    }
    
    std::string network = argv[1];
    
    if (network == "mainnet") {
        PrintParams(MAINNET_PARAMS);
    } else if (network == "testnet") {
        PrintParams(TESTNET_PARAMS);
    } else if (network == "regtest") {
        PrintParams(REGTEST_PARAMS);
    } else {
        std::cerr << "Unknown network: " << network << std::endl;
        return 1;
    }
    
    return 0;
}
