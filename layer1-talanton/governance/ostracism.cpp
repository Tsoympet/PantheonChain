#include "ostracism.h"

namespace parthenon {
namespace governance {

Ostracism::Ostracism(uint64_t ban_duration_blocks, uint64_t required_votes_for)
    : ban_duration_blocks_(ban_duration_blocks),
      required_votes_for_(required_votes_for) {}

bool Ostracism::Nominate(const std::vector<uint8_t>& subject,
                         const std::vector<uint8_t>& nominator,
                         const std::string& reason,
                         uint64_t block_height) {
    if (subject.empty() || nominator.empty()) return false;
    if (subject == nominator) return false;  // Cannot self-nominate

    auto it = records_.find(subject);
    if (it != records_.end()) {
        // Re-nomination only allowed after rehabilitation
        if (it->second.state != State::REHABILITATED) return false;
    }

    Record rec;
    rec.subject = subject;
    rec.nominator = nominator;
    rec.reason = reason;
    rec.nominated_at_block = block_height;
    rec.ostracized_at_block = 0;
    rec.ban_end_block = 0;
    rec.votes_for = 0;
    rec.votes_against = 0;
    rec.state = State::NOMINATED;
    records_[subject] = rec;
    voters_[subject].clear();
    return true;
}

bool Ostracism::Vote(const std::vector<uint8_t>& subject,
                     const std::vector<uint8_t>& voter,
                     bool vote_to_ostracize,
                     uint64_t /*block_height*/) {
    auto it = records_.find(subject);
    if (it == records_.end()) return false;
    if (it->second.state != State::NOMINATED) return false;

    auto& voted = voters_[subject];
    if (voted.count(voter)) return false;  // Already voted

    voted.insert(voter);
    if (vote_to_ostracize) {
        ++it->second.votes_for;
    } else {
        ++it->second.votes_against;
    }
    return true;
}

bool Ostracism::HasVoted(const std::vector<uint8_t>& subject,
                         const std::vector<uint8_t>& voter) const {
    auto it = voters_.find(subject);
    if (it == voters_.end()) return false;
    return it->second.count(voter) > 0;
}

bool Ostracism::Finalize(const std::vector<uint8_t>& subject, uint64_t block_height) {
    auto it = records_.find(subject);
    if (it == records_.end()) return false;
    if (it->second.state != State::NOMINATED) return false;
    if (it->second.votes_for < required_votes_for_) return false;

    it->second.state = State::OSTRACIZED;
    it->second.ostracized_at_block = block_height;
    it->second.ban_end_block = block_height + ban_duration_blocks_;
    return true;
}

bool Ostracism::IsOstracized(const std::vector<uint8_t>& address,
                              uint64_t block_height) const {
    auto it = records_.find(address);
    if (it == records_.end()) return false;
    if (it->second.state != State::OSTRACIZED) return false;
    return block_height < it->second.ban_end_block;
}

bool Ostracism::IsNominated(const std::vector<uint8_t>& address) const {
    auto it = records_.find(address);
    return it != records_.end() && it->second.state == State::NOMINATED;
}

std::optional<Ostracism::Record>
Ostracism::GetRecord(const std::vector<uint8_t>& address) const {
    auto it = records_.find(address);
    if (it == records_.end()) return std::nullopt;
    return it->second;
}

bool Ostracism::Rehabilitate(const std::vector<uint8_t>& address,
                              uint64_t block_height) {
    auto it = records_.find(address);
    if (it == records_.end()) return false;
    if (it->second.state != State::OSTRACIZED) return false;
    if (block_height < it->second.ban_end_block) return false;

    it->second.state = State::REHABILITATED;
    return true;
}

}  // namespace governance
}  // namespace parthenon
