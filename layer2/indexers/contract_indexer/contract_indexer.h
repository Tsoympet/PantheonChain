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
    
private:
    // TODO: Implement database backend
};

} // namespace indexers
} // namespace layer2
} // namespace parthenon
