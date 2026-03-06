// ParthenonChain - Balance-Based Voting Registry
// Token holders vote with their wallet balance; no staking required.

#ifndef PARTHENON_GOVERNANCE_BALANCE_VOTING_H
#define PARTHENON_GOVERNANCE_BALANCE_VOTING_H

#include <cstdint>
#include <map>
#include <vector>
#include <utility>

namespace parthenon {
namespace governance {

/**
 * BalanceVotingRegistry
 *
 * Derives governance voting power directly from token balances rather
 * than from staked amounts.  Any token holder can participate in
 * governance without locking tokens first.
 *
 * Usage
 * -----
 *  1. After each block, call SetBalances() with the current on-chain
 *     balance snapshot (address → token amount).
 *  2. When VotingSystem::CreateProposal() fires, call GetAllVotingPowers()
 *     to produce the snapshot input.
 *  3. CastVote() uses the snapshot power for that specific proposal.
 *
 * Interface compatibility
 * -----------------------
 *  The class mirrors the query-side API of StakingRegistry so that
 *  VotingSystem can use either source transparently.
 */
class BalanceVotingRegistry {
  public:
    BalanceVotingRegistry() = default;

    // ------------------------------------------------------------------ //
    //  Balance management                                                  //
    // ------------------------------------------------------------------ //

    /**
     * Replace the entire balance map with a fresh snapshot.
     * Typically called once per block after the UTXO set is updated.
     */
    void SetBalances(const std::map<std::vector<uint8_t>, uint64_t>& balances);

    /**
     * Update or insert the balance for a single address.
     */
    void UpdateBalance(const std::vector<uint8_t>& address, uint64_t amount);

    // ------------------------------------------------------------------ //
    //  Voting-power queries (same interface as StakingRegistry)            //
    // ------------------------------------------------------------------ //

    /**
     * Returns the current token balance for address (= its voting power).
     */
    uint64_t GetVotingPower(const std::vector<uint8_t>& address) const;

    /**
     * Sum of all voting-eligible balances.
     */
    uint64_t GetTotalVotingPower() const;

    /**
     * Snapshot-ready vector of (address, voting_power) pairs for every
     * holder with a non-zero balance.
     */
    std::vector<std::pair<std::vector<uint8_t>, uint64_t>> GetAllVotingPowers() const;

    /**
     * Number of tracked addresses.
     */
    size_t Size() const { return balances_.size(); }

  private:
    // address → token balance (= voting power)
    std::map<std::vector<uint8_t>, uint64_t> balances_;
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_BALANCE_VOTING_H
