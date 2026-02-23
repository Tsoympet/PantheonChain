#include "emergency.h"

#include <algorithm>
#include <map>

namespace parthenon {
namespace governance {

EmergencyCouncil::EmergencyCouncil(uint32_t required_sigs,
                                   uint64_t action_ttl_blocks)
    : required_sigs_(required_sigs), action_ttl_(action_ttl_blocks) {}

// ---------------------------------------------------------------------------
// Guardians
// ---------------------------------------------------------------------------

bool EmergencyCouncil::AddGuardian(const std::vector<uint8_t>& address,
                                    const std::string& role,
                                    uint64_t block_height) {
    if (address.empty()) return false;
    if (IsGuardian(address)) return false;

    Guardian g;
    g.address       = address;
    g.role          = role;
    g.added_at_block = block_height;
    guardians_.push_back(g);
    return true;
}

bool EmergencyCouncil::RemoveGuardian(const std::vector<uint8_t>& address) {
    auto it = std::find_if(guardians_.begin(), guardians_.end(),
                           [&](const Guardian& g) { return g.address == address; });
    if (it == guardians_.end()) return false;
    guardians_.erase(it);
    return true;
}

bool EmergencyCouncil::IsGuardian(const std::vector<uint8_t>& address) const {
    return std::any_of(guardians_.begin(), guardians_.end(),
                       [&](const Guardian& g) { return g.address == address; });
}

std::vector<EmergencyCouncil::Guardian> EmergencyCouncil::GetGuardians() const {
    return guardians_;
}

// ---------------------------------------------------------------------------
// Action lifecycle
// ---------------------------------------------------------------------------

uint64_t EmergencyCouncil::ProposeAction(ActionType type,
                                          const std::string& description,
                                          const std::vector<uint8_t>& initiator,
                                          uint64_t target_proposal_id,
                                          uint64_t block_height) {
    if (!IsGuardian(initiator)) return 0;

    Action action;
    action.action_id            = next_action_id_++;
    action.type                 = type;
    action.description          = description;
    action.initiator            = initiator;
    action.target_proposal_id   = target_proposal_id;
    action.proposed_at_block    = block_height;
    action.expires_at_block     = block_height + action_ttl_;
    action.executed             = false;
    action.executed_at_block    = 0;
    action.signers.insert(initiator);  // initiator counts as first signature

    actions_[action.action_id] = action;
    return action.action_id;
}

bool EmergencyCouncil::SignAction(uint64_t action_id,
                                   const std::vector<uint8_t>& guardian,
                                   uint64_t block_height) {
    if (!IsGuardian(guardian)) return false;

    auto it = actions_.find(action_id);
    if (it == actions_.end()) return false;

    Action& action = it->second;
    if (action.executed) return false;
    if (IsExpired(action_id, block_height)) return false;

    action.signers.insert(guardian);
    return true;
}

bool EmergencyCouncil::ExecuteAction(uint64_t action_id, uint64_t block_height) {
    auto it = actions_.find(action_id);
    if (it == actions_.end()) return false;

    Action& action = it->second;
    if (action.executed) return false;
    if (IsExpired(action_id, block_height)) return false;
    if (!HasSufficientSignatures(action_id)) return false;

    action.executed          = true;
    action.executed_at_block = block_height;

    // Apply side effects
    switch (action.type) {
        case ActionType::PAUSE_GOVERNANCE:
            governance_paused_ = true;
            break;
        case ActionType::CANCEL_PROPOSAL:
            // Side effect: callers check GetAction() and act accordingly
            break;
        case ActionType::FAST_TRACK_UPGRADE:
            // Side effect: callers check GetAction() and act accordingly
            break;
        case ActionType::CUSTOM:
            break;
    }

    return true;
}

bool EmergencyCouncil::HasSufficientSignatures(uint64_t action_id) const {
    auto it = actions_.find(action_id);
    if (it == actions_.end()) return false;
    return static_cast<uint32_t>(it->second.signers.size()) >= required_sigs_;
}

bool EmergencyCouncil::IsExpired(uint64_t action_id,
                                  uint64_t block_height) const {
    auto it = actions_.find(action_id);
    if (it == actions_.end()) return true;
    return block_height > it->second.expires_at_block;
}

std::optional<EmergencyCouncil::Action>
EmergencyCouncil::GetAction(uint64_t action_id) const {
    auto it = actions_.find(action_id);
    if (it == actions_.end()) return std::nullopt;
    return it->second;
}

}  // namespace governance
}  // namespace parthenon
