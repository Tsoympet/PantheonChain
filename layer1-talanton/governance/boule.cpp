#include "boule.h"

#include <algorithm>
#include <numeric>

namespace parthenon {
namespace governance {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

Boule::Boule(uint32_t council_size, uint64_t term_blocks,
             uint64_t min_stake, bool screening_required)
    : council_size_(council_size),
      term_blocks_(term_blocks),
      min_stake_(min_stake),
      screening_required_(screening_required) {}

// ---------------------------------------------------------------------------
// Citizen registry
// ---------------------------------------------------------------------------

bool Boule::RegisterCitizen(const std::vector<uint8_t>& address,
                            uint64_t stake_amount,
                            uint64_t block_height) {
    if (address.empty()) return false;
    if (stake_amount < min_stake_) return false;

    // Prevent duplicate registration
    for (const auto& c : citizens_) {
        if (c.address == address) return false;
    }

    Citizen citizen;
    citizen.address = address;
    citizen.registered_at_block = block_height;
    citizen.stake_amount = stake_amount;
    citizen.is_eligible = true;
    citizens_.push_back(citizen);
    return true;
}

bool Boule::SetCitizenEligibility(const std::vector<uint8_t>& address, bool eligible) {
    for (auto& c : citizens_) {
        if (c.address == address) {
            c.is_eligible = eligible;
            return true;
        }
    }
    return false;
}

bool Boule::IsCitizenRegistered(const std::vector<uint8_t>& address) const {
    for (const auto& c : citizens_) {
        if (c.address == address) return true;
    }
    return false;
}

bool Boule::IsCouncilMember(const std::vector<uint8_t>& address) const {
    for (const auto& m : council_) {
        if (m.address == address) return true;
    }
    return false;
}

std::vector<Boule::Citizen> Boule::GetCitizens() const { return citizens_; }

std::vector<Boule::CouncilMember> Boule::GetCurrentCouncil() const { return council_; }

// ---------------------------------------------------------------------------
// Kleroterion – sortition
// ---------------------------------------------------------------------------

bool Boule::ConductSortition(const std::vector<uint8_t>& seed, uint64_t block_height) {
    if (seed.size() < 4) return false;

    // Collect eligible citizens
    std::vector<size_t> eligible;
    for (size_t i = 0; i < citizens_.size(); ++i) {
        if (citizens_[i].is_eligible) eligible.push_back(i);
    }

    if (eligible.size() < council_size_) return false;

    // Deterministic shuffle using the seed (Fisher-Yates with seed-derived PRNG).
    // Simple LCG seeded from first 4 bytes of seed; sufficient for sortition.
    uint32_t rng = (static_cast<uint32_t>(seed[0]) << 24) |
                   (static_cast<uint32_t>(seed[1]) << 16) |
                   (static_cast<uint32_t>(seed[2]) <<  8) |
                    static_cast<uint32_t>(seed[3]);

    auto lcg_next = [&rng]() -> uint32_t {
        rng = rng * 1664525u + 1013904223u;  // Numerical Recipes LCG
        return rng;
    };

    for (size_t i = eligible.size() - 1; i > 0; --i) {
        uint32_t j = lcg_next() % static_cast<uint32_t>(i + 1);
        std::swap(eligible[i], eligible[j]);
    }

    // Select first council_size_ after shuffle
    council_.clear();
    for (uint32_t k = 0; k < council_size_; ++k) {
        const Citizen& c = citizens_[eligible[k]];
        CouncilMember m;
        m.address = c.address;
        m.selected_at_block = block_height;
        m.term_end_block = block_height + term_blocks_;
        m.selection_index = k;
        council_.push_back(m);
    }

    return true;
}

// ---------------------------------------------------------------------------
// Proposal screening
// ---------------------------------------------------------------------------

bool Boule::ReviewProposal(uint64_t proposal_id,
                           const std::vector<uint8_t>& council_member,
                           bool approved,
                           const std::string& rationale,
                           uint64_t block_height) {
    if (!IsCouncilMember(council_member)) return false;

    // Prevent duplicate review by same member
    auto& rev_list = reviews_[proposal_id];
    for (const auto& r : rev_list) {
        if (r.reviewer == council_member) return false;
    }

    ProposalReview review;
    review.proposal_id = proposal_id;
    review.reviewer = council_member;
    review.approved = approved;
    review.rationale = rationale;
    review.reviewed_at_block = block_height;
    rev_list.push_back(review);
    return true;
}

bool Boule::IsProposalApproved(uint64_t proposal_id) const {
    // If screening is not required, proposals are always considered approved.
    if (!screening_required_) return true;

    // An active Graphe Paranomon challenge blocks the proposal.
    auto ch_it = challenges_.find(proposal_id);
    if (ch_it != challenges_.end() && !ch_it->second.resolved) return false;

    auto it = reviews_.find(proposal_id);
    if (it == reviews_.end() || council_.empty()) return false;

    uint64_t approve_count = 0;
    for (const auto& r : it->second) {
        if (r.approved) ++approve_count;
    }

    // Require >= 2/3 of council_size_ approvals
    uint64_t required = (static_cast<uint64_t>(council_size_) * 2 + 2) / 3;
    return approve_count >= required;
}

std::vector<Boule::ProposalReview>
Boule::GetProposalReviews(uint64_t proposal_id) const {
    auto it = reviews_.find(proposal_id);
    if (it == reviews_.end()) return {};
    return it->second;
}

// ---------------------------------------------------------------------------
// Graphe Paranomon
// ---------------------------------------------------------------------------

bool Boule::RaiseGrapheParanomon(uint64_t proposal_id,
                                 const std::vector<uint8_t>& challenger,
                                 const std::string& grounds,
                                 uint64_t block_height) {
    if (!IsCouncilMember(challenger)) return false;

    // Only one active challenge per proposal
    auto it = challenges_.find(proposal_id);
    if (it != challenges_.end() && !it->second.resolved) return false;

    GrapheParanomon challenge;
    challenge.proposal_id = proposal_id;
    challenge.challenger = challenger;
    challenge.grounds = grounds;
    challenge.raised_at_block = block_height;
    challenge.resolved = false;
    challenge.upheld = false;
    challenge.dismiss_votes = 0;
    challenge.uphold_votes = 0;
    challenges_[proposal_id] = challenge;
    return true;
}

bool Boule::VoteOnGrapheParanomon(uint64_t proposal_id,
                                  const std::vector<uint8_t>& council_member,
                                  bool dismiss,
                                  uint64_t /*block_height*/) {
    if (!IsCouncilMember(council_member)) return false;

    auto it = challenges_.find(proposal_id);
    if (it == challenges_.end() || it->second.resolved) return false;

    GrapheParanomon& ch = it->second;
    if (dismiss) {
        ++ch.dismiss_votes;
    } else {
        ++ch.uphold_votes;
    }

    // Resolve when a majority direction has enough votes (> half council)
    uint64_t majority = static_cast<uint64_t>(council_size_) / 2 + 1;
    if (ch.dismiss_votes >= majority) {
        ch.resolved = true;
        ch.upheld = false;
    } else if (ch.uphold_votes >= majority) {
        ch.resolved = true;
        ch.upheld = true;
    }

    return true;
}

bool Boule::HasActiveChallenge(uint64_t proposal_id) const {
    auto it = challenges_.find(proposal_id);
    return it != challenges_.end() && !it->second.resolved;
}

std::optional<Boule::GrapheParanomon>
Boule::GetChallenge(uint64_t proposal_id) const {
    auto it = challenges_.find(proposal_id);
    if (it == challenges_.end()) return std::nullopt;
    return it->second;
}

// ---------------------------------------------------------------------------
// Prytany
// ---------------------------------------------------------------------------

std::vector<Boule::CouncilMember> Boule::GetPrytany() const {
    if (council_.empty()) return {};

    uint32_t prytany_size = std::max(1u, council_size_ / 10u);
    std::vector<CouncilMember> prytany;

    // Sorted copy by selection_index
    auto sorted = council_;
    std::sort(sorted.begin(), sorted.end(),
              [](const CouncilMember& a, const CouncilMember& b) {
                  return a.selection_index < b.selection_index;
              });

    for (uint32_t i = 0; i < prytany_size && i < sorted.size(); ++i) {
        prytany.push_back(sorted[i]);
    }
    return prytany;
}

bool Boule::IsPrytanyMember(const std::vector<uint8_t>& address) const {
    for (const auto& m : GetPrytany()) {
        if (m.address == address) return true;
    }
    return false;
}

}  // namespace governance
}  // namespace parthenon
