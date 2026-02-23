#include "params.h"

namespace parthenon {
namespace governance {

// ---------------------------------------------------------------------------
// Defaults
// ---------------------------------------------------------------------------

GovernanceParams::Params GovernanceParams::Defaults() {
    Params p{};
    p.voting_delay_blocks          = 100;
    p.voting_period_blocks         = 10000;
    p.execution_delay_blocks       = 1000;
    p.max_proposal_age_blocks      = 50400;  // ~7 days

    p.default_quorum               = 1000000;
    p.default_threshold_bps        = 5000;   // 50 %
    p.constitutional_threshold_bps = 6667;   // ~2/3

    p.min_proposal_deposit         = 0;
    p.slash_deposit_on_rejection   = false;
    p.slash_deposit_on_spam        = false;

    p.quadratic_voting_enabled     = true;
    p.max_voting_power_cap         = 0;       // disabled
    p.whale_threshold_bps          = 1000;    // 10 %

    p.boule_size                   = 21;
    p.boule_term_blocks            = 50400;
    p.boule_min_stake              = 0;
    p.boule_screening_required     = true;

    p.ostracism_ban_duration_blocks = 50400;
    p.ostracism_required_votes      = 10;

    return p;
}

GovernanceParams::GovernanceParams(const Params& initial) : params_(initial) {}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

bool GovernanceParams::ValidateUint(const std::string& key, uint64_t value) const {
    if (key == "voting_period_blocks") {
        return value >= kLimits.min_voting_period_blocks &&
               value <= kLimits.max_voting_period_blocks;
    }
    if (key == "constitutional_threshold_bps") {
        return value >= kLimits.min_constitutional_threshold && value <= 10000;
    }
    if (key == "default_threshold_bps") {
        return value >= kLimits.min_default_threshold && value <= 10000;
    }
    if (key == "boule_size") {
        return value >= 1 && value <= kLimits.max_boule_size;
    }
    if (key == "voting_delay_blocks" || key == "execution_delay_blocks") {
        return value > 0;
    }
    return true;  // No specific constitutional limit; any value accepted.
}

// ---------------------------------------------------------------------------
// UpdateParam
// ---------------------------------------------------------------------------

bool GovernanceParams::UpdateParam(const std::string& key, uint64_t value,
                                   uint64_t proposal_id, uint64_t block_height) {
    if (proposal_id == 0) return false;
    if (!ValidateUint(key, value)) return false;

    uint64_t old_value = 0;
    bool found = true;

    if      (key == "voting_delay_blocks")          { old_value = params_.voting_delay_blocks;          params_.voting_delay_blocks          = value; }
    else if (key == "voting_period_blocks")         { old_value = params_.voting_period_blocks;         params_.voting_period_blocks         = value; }
    else if (key == "execution_delay_blocks")       { old_value = params_.execution_delay_blocks;       params_.execution_delay_blocks       = value; }
    else if (key == "max_proposal_age_blocks")      { old_value = params_.max_proposal_age_blocks;      params_.max_proposal_age_blocks      = value; }
    else if (key == "default_quorum")               { old_value = params_.default_quorum;               params_.default_quorum               = value; }
    else if (key == "default_threshold_bps")        { old_value = params_.default_threshold_bps;        params_.default_threshold_bps        = value; }
    else if (key == "constitutional_threshold_bps") { old_value = params_.constitutional_threshold_bps; params_.constitutional_threshold_bps = value; }
    else if (key == "min_proposal_deposit")         { old_value = params_.min_proposal_deposit;         params_.min_proposal_deposit         = value; }
    else if (key == "max_voting_power_cap")         { old_value = params_.max_voting_power_cap;         params_.max_voting_power_cap         = value; }
    else if (key == "whale_threshold_bps")          { old_value = params_.whale_threshold_bps;          params_.whale_threshold_bps          = value; }
    else if (key == "boule_size")                   { old_value = params_.boule_size;                   params_.boule_size                   = static_cast<uint32_t>(value); }
    else if (key == "boule_term_blocks")            { old_value = params_.boule_term_blocks;             params_.boule_term_blocks            = value; }
    else if (key == "boule_min_stake")              { old_value = params_.boule_min_stake;               params_.boule_min_stake              = value; }
    else if (key == "ostracism_ban_duration_blocks"){ old_value = params_.ostracism_ban_duration_blocks; params_.ostracism_ban_duration_blocks= value; }
    else if (key == "ostracism_required_votes")     { old_value = params_.ostracism_required_votes;      params_.ostracism_required_votes     = value; }
    else { found = false; }

    if (!found) return false;

    ParamChange change;
    change.key             = key;
    change.old_value       = old_value;
    change.new_value       = value;
    change.proposal_id     = proposal_id;
    change.changed_at_block = block_height;
    history_.push_back(change);
    return true;
}

bool GovernanceParams::UpdateBoolParam(const std::string& key, bool value,
                                       uint64_t proposal_id, uint64_t block_height) {
    if (proposal_id == 0) return false;

    uint64_t old_val = 0;
    bool found = true;

    if      (key == "quadratic_voting_enabled")     { old_val = params_.quadratic_voting_enabled     ? 1 : 0; params_.quadratic_voting_enabled     = value; }
    else if (key == "slash_deposit_on_rejection")   { old_val = params_.slash_deposit_on_rejection   ? 1 : 0; params_.slash_deposit_on_rejection   = value; }
    else if (key == "slash_deposit_on_spam")        { old_val = params_.slash_deposit_on_spam        ? 1 : 0; params_.slash_deposit_on_spam        = value; }
    else if (key == "boule_screening_required")     { old_val = params_.boule_screening_required     ? 1 : 0; params_.boule_screening_required     = value; }
    else { found = false; }

    if (!found) return false;

    ParamChange change;
    change.key              = key;
    change.old_value        = old_val;
    change.new_value        = value ? 1 : 0;
    change.proposal_id      = proposal_id;
    change.changed_at_block = block_height;
    history_.push_back(change);
    return true;
}

}  // namespace governance
}  // namespace parthenon
