// ParthenonChain - Decentralized Governance Protocol
// Parameter updates and protocol upgrades WITHOUT voting mechanisms

#pragma once

#include <vector>
#include <string>
#include <map>
#include <optional>
#include <cstdint>

namespace parthenon {
namespace governance {

/**
 * Governance parameter types
 */
enum class ParameterType {
    BLOCK_SIZE_LIMIT,
    BLOCK_TIME,
    MIN_TX_FEE,
    MAX_ROLLUP_BATCH_SIZE,
    BRIDGE_FEE,
    CHALLENGE_PERIOD,
    GAS_LIMIT
};

/**
 * Governance proposal (parameter update only)
 */
struct Proposal {
    uint64_t proposal_id;
    std::string title;
    std::string description;
    ParameterType parameter;
    uint64_t current_value;
    uint64_t proposed_value;
    uint64_t submission_time;
    uint64_t execution_time;      // When proposal can be executed
    uint64_t timelock_duration;   // Required delay before execution
    bool executed;
    std::vector<uint8_t> proposer_signature;
    
    Proposal() : proposal_id(0), parameter(ParameterType::BLOCK_SIZE_LIMIT)
               , current_value(0), proposed_value(0)
               , submission_time(0), execution_time(0)
               , timelock_duration(0), executed(false) {}
};

/**
 * Governance Protocol
 * Manages protocol parameters and upgrades
 */
class GovernanceProtocol {
public:
    GovernanceProtocol();
    ~GovernanceProtocol();
    
    /**
     * Submit parameter update proposal
     */
    uint64_t SubmitProposal(const Proposal& proposal);
    
    /**
     * Execute proposal after timelock
     */
    bool ExecuteProposal(uint64_t proposal_id);
    
    /**
     * Get proposal by ID
     */
    std::optional<Proposal> GetProposal(uint64_t proposal_id) const;
    
    /**
     * Get all pending proposals
     */
    std::vector<Proposal> GetPendingProposals() const;
    
    /**
     * Get current parameter value
     */
    uint64_t GetParameter(ParameterType param) const;
    
    /**
     * Set parameter (internal - called after proposal execution)
     */
    bool SetParameter(ParameterType param, uint64_t value);
    
    /**
     * Get timelock duration for parameter type
     */
    uint64_t GetTimelockDuration(ParameterType param) const;
    
    /**
     * Check if proposal can be executed
     */
    bool CanExecute(uint64_t proposal_id, uint64_t current_time) const;
    
private:
    uint64_t next_proposal_id_;
    std::map<uint64_t, Proposal> proposals_;
    std::map<ParameterType, uint64_t> parameters_;
    std::map<ParameterType, uint64_t> timelock_durations_;
};

/**
 * Protocol Upgrade Manager
 * Manages protocol version upgrades
 */
class ProtocolUpgradeManager {
public:
    struct UpgradeProposal {
        uint64_t upgrade_id;
        uint32_t from_version;
        uint32_t to_version;
        std::string description;
        std::vector<uint8_t> upgrade_code_hash;
        uint64_t activation_height;
        bool activated;
        
        UpgradeProposal() : upgrade_id(0), from_version(0), to_version(0)
                          , activation_height(0), activated(false) {}
    };
    
    /**
     * Propose protocol upgrade
     */
    uint64_t ProposeUpgrade(const UpgradeProposal& upgrade);
    
    /**
     * Activate upgrade at specified height
     */
    bool ActivateUpgrade(uint64_t upgrade_id, uint64_t current_height);
    
    /**
     * Get current protocol version
     */
    uint32_t GetCurrentVersion() const { return current_version_; }
    
    /**
     * Check if upgrade is pending
     */
    bool IsUpgradePending(uint64_t height) const;
    
private:
    uint32_t current_version_;
    std::map<uint64_t, UpgradeProposal> upgrades_;
};

/**
 * Emergency Actions
 * Emergency protocol actions (pause/unpause)
 */
class EmergencyActions {
public:
    /**
     * Pause protocol (in case of critical bug)
     */
    bool PauseProtocol(const std::vector<uint8_t>& admin_signature);
    
    /**
     * Unpause protocol
     */
    bool UnpauseProtocol(const std::vector<uint8_t>& admin_signature);
    
    /**
     * Check if protocol is paused
     */
    bool IsPaused() const { return paused_; }
    
    /**
     * Set emergency admin (multi-sig address)
     */
    void SetEmergencyAdmin(const std::vector<uint8_t>& admin_address);
    
private:
    bool paused_;
    std::vector<uint8_t> emergency_admin_;
};

} // namespace governance
} // namespace parthenon
