#include "boule.h"

#include "../core/crypto/sha256.h"

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

    // Deterministic Fisher-Yates shuffle using a SHA256-based VRF.
    // Each draw hashes (seed || block_height_le8 || counter_le8) to produce
    // 32 bytes of unpredictable-but-deterministic randomness.  Using the full
    // hash output (not a truncated LCG) ensures uniform, bias-resistant draws.
    auto append_le64 = [](std::vector<uint8_t>& buf, uint64_t v) {
        for (int bi = 0; bi < 8; ++bi)
            buf.push_back(static_cast<uint8_t>((v >> (8 * bi)) & 0xFF));
    };
    uint64_t vrf_counter = 0;
    auto vrf_next = [&]() -> uint32_t {
        std::vector<uint8_t> vrf_input;
        vrf_input.insert(vrf_input.end(), seed.begin(), seed.end());
        append_le64(vrf_input, block_height);
        append_le64(vrf_input, vrf_counter);
        ++vrf_counter;
        auto hash = crypto::SHA256::Hash256(vrf_input.data(), vrf_input.size());
        // Read first 4 bytes of hash as little-endian uint32 for portability.
        return static_cast<uint32_t>(hash[0])        |
               (static_cast<uint32_t>(hash[1]) << 8) |
               (static_cast<uint32_t>(hash[2]) << 16)|
               (static_cast<uint32_t>(hash[3]) << 24);
    };

    for (size_t i = eligible.size() - 1; i > 0; --i) {
        uint32_t j = vrf_next() % static_cast<uint32_t>(i + 1);
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
