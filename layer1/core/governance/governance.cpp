// ParthenonChain - Governance Implementation

#include "governance.h"

namespace parthenon {
namespace governance {

// GovernanceProtocol implementation
GovernanceProtocol::GovernanceProtocol() : next_proposal_id_(1) {
    // Initialize default parameters
    parameters_[ParameterType::BLOCK_SIZE_LIMIT] = 8000000;  // 8MB
    parameters_[ParameterType::BLOCK_TIME] = 10;              // 10 seconds
    parameters_[ParameterType::MIN_TX_FEE] = 1000;           // 1000 units
    parameters_[ParameterType::MAX_ROLLUP_BATCH_SIZE] = 1000;
    parameters_[ParameterType::BRIDGE_FEE] = 100;             // 100 basis points
    parameters_[ParameterType::CHALLENGE_PERIOD] = 1008;      // ~1 week in blocks
    parameters_[ParameterType::GAS_LIMIT] = 30000000;
    
    // Initialize timelock durations (in blocks)
    timelock_durations_[ParameterType::BLOCK_SIZE_LIMIT] = 4032;   // ~1 week
    timelock_durations_[ParameterType::BLOCK_TIME] = 8064;          // ~2 weeks
    timelock_durations_[ParameterType::MIN_TX_FEE] = 2016;          // ~3.5 days
    timelock_durations_[ParameterType::MAX_ROLLUP_BATCH_SIZE] = 2016;
    timelock_durations_[ParameterType::BRIDGE_FEE] = 2016;
    timelock_durations_[ParameterType::CHALLENGE_PERIOD] = 4032;
    timelock_durations_[ParameterType::GAS_LIMIT] = 4032;
}

GovernanceProtocol::~GovernanceProtocol() {}

uint64_t GovernanceProtocol::SubmitProposal(const Proposal& proposal) {
    Proposal new_proposal = proposal;
    new_proposal.proposal_id = next_proposal_id_++;
    new_proposal.current_value = GetParameter(proposal.parameter);
    new_proposal.timelock_duration = GetTimelockDuration(proposal.parameter);
    new_proposal.execution_time = proposal.submission_time + new_proposal.timelock_duration;
    new_proposal.executed = false;
    
    proposals_[new_proposal.proposal_id] = new_proposal;
    return new_proposal.proposal_id;
}

bool GovernanceProtocol::ExecuteProposal(uint64_t proposal_id) {
    auto it = proposals_.find(proposal_id);
    if (it == proposals_.end()) {
        return false;
    }
    
    if (it->second.executed) {
        return false;
    }
    
    it->second.executed = true;
    return SetParameter(it->second.parameter, it->second.proposed_value);
}

std::optional<Proposal> GovernanceProtocol::GetProposal(uint64_t proposal_id) const {
    auto it = proposals_.find(proposal_id);
    if (it == proposals_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<Proposal> GovernanceProtocol::GetPendingProposals() const {
    std::vector<Proposal> pending;
    for (const auto& [id, proposal] : proposals_) {
        if (!proposal.executed) {
            pending.push_back(proposal);
        }
    }
    return pending;
}

uint64_t GovernanceProtocol::GetParameter(ParameterType param) const {
    auto it = parameters_.find(param);
    if (it == parameters_.end()) {
        return 0;
    }
    return it->second;
}

bool GovernanceProtocol::SetParameter(ParameterType param, uint64_t value) {
    parameters_[param] = value;
    return true;
}

uint64_t GovernanceProtocol::GetTimelockDuration(ParameterType param) const {
    auto it = timelock_durations_.find(param);
    if (it == timelock_durations_.end()) {
        return 2016;  // Default ~3.5 days
    }
    return it->second;
}

bool GovernanceProtocol::CanExecute(uint64_t proposal_id, uint64_t current_time) const {
    auto it = proposals_.find(proposal_id);
    if (it == proposals_.end()) {
        return false;
    }
    
    return !it->second.executed && current_time >= it->second.execution_time;
}

// ProtocolUpgradeManager implementation
uint64_t ProtocolUpgradeManager::ProposeUpgrade(const UpgradeProposal& upgrade) {
    UpgradeProposal new_upgrade = upgrade;
    new_upgrade.from_version = current_version_;
    new_upgrade.activated = false;
    
    upgrades_[new_upgrade.upgrade_id] = new_upgrade;
    return new_upgrade.upgrade_id;
}

bool ProtocolUpgradeManager::ActivateUpgrade(uint64_t upgrade_id, uint64_t current_height) {
    auto it = upgrades_.find(upgrade_id);
    if (it == upgrades_.end()) {
        return false;
    }
    
    if (current_height < it->second.activation_height) {
        return false;
    }
    
    it->second.activated = true;
    current_version_ = it->second.to_version;
    return true;
}

bool ProtocolUpgradeManager::IsUpgradePending(uint64_t height) const {
    for (const auto& [id, upgrade] : upgrades_) {
        if (!upgrade.activated && height >= upgrade.activation_height) {
            return true;
        }
    }
    return false;
}

// EmergencyActions implementation
bool EmergencyActions::PauseProtocol([[maybe_unused]] const std::vector<uint8_t>& admin_signature) {
    // In production: verify admin signature
    paused_ = true;
    return true;
}

bool EmergencyActions::UnpauseProtocol([[maybe_unused]] const std::vector<uint8_t>& admin_signature) {
    // In production: verify admin signature
    paused_ = false;
    return true;
}

void EmergencyActions::SetEmergencyAdmin(const std::vector<uint8_t>& admin_address) {
    emergency_admin_ = admin_address;
}

} // namespace governance
} // namespace parthenon
