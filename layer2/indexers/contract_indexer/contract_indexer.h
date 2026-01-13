// ParthenonChain - Contract Indexer
// Indexes EVM contract deployments and events

#pragma once

#include "evm/state.h"
#include <string>
#include <vector>

namespace parthenon {
namespace layer2 {
namespace indexers {

/**
 * Contract Event
 */
struct ContractEvent {
    std::array<uint8_t, 20> contract_address;
    std::vector<std::array<uint8_t, 32>> topics;
    std::vector<uint8_t> data;
    uint32_t block_height;
    uint32_t tx_index;
};

/**
 * Contract Indexer
 * 
 * Indexes EVM smart contracts and their events
 */
class ContractIndexer {
public:
    ContractIndexer();
    ~ContractIndexer();
    
    /**
     * Open the indexer database
     */
    bool Open(const std::string& db_path);
    
    /**
     * Close the indexer database
     */
    void Close();
    
    /**
     * Index a contract deployment
     */
    void IndexContractDeployment(
        const std::array<uint8_t, 20>& address,
        const std::vector<uint8_t>& code,
        uint32_t height
    );
    
    /**
     * Index a contract event
     */
    void IndexEvent(const ContractEvent& event);
    
    /**
     * Get events for a contract
     */
    std::vector<ContractEvent> GetEventsByContract(
        const std::array<uint8_t, 20>& contract_address,
        uint32_t limit = 100
    );
    
    /**
     * Get events by topic (event signature)
     */
    std::vector<ContractEvent> GetEventsByTopic(
        const std::array<uint8_t, 32>& topic,
        uint32_t limit = 100
    );
    
    /**
     * Contract information
     */
    struct ContractInfo {
        std::array<uint8_t, 20> address;
        std::vector<uint8_t> code;
        uint32_t deployment_height;
        uint64_t event_count;
    };
    
    /**
     * Get contract information
     */
    std::optional<ContractInfo> GetContractInfo(
        const std::array<uint8_t, 20>& address
    );
    
    /**
     * Get total number of indexed contracts
     */
    size_t GetContractCount() const;
    
    /**
     * Get total number of indexed events
     */
    size_t GetEventCount() const;
    
    /**
     * Get all contract addresses
     */
    std::vector<std::array<uint8_t, 20>> GetAllContracts() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace indexers
} // namespace layer2
} // namespace parthenon
