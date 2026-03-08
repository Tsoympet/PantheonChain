// ParthenonChain - One-Address-One-Vote Registry
// Every token holder — regardless of how many tokens they own — gets
// exactly 1 vote.  Holding more tokens does not grant more influence.

#ifndef PARTHENON_GOVERNANCE_BALANCE_VOTING_H
#define PARTHENON_GOVERNANCE_BALANCE_VOTING_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

namespace parthenon {
namespace governance {

/**
 * BalanceVotingRegistry  (One-Address-One-Vote model)
 *
 * Every address holding at least 1 token of any PantheonChain asset is
 * eligible to vote in governance.  Its voting power is always exactly 1,
 * regardless of the size of its balance.  This prevents wealthy holders
 * from dominating governance decisions.
 *
 * Model
 * -----
 *  voting_power(address) = 1   if balance(address) > 0
 *                        = 0   otherwise
 *  total_voting_power          = number of addresses with balance > 0
 *
 * Usage
 * -----
 *  1. After each block, call SetBalances() with the current on-chain
 *     balance snapshot (address → token amount, any asset).
 *  2. When VotingSystem::CreateProposal() fires, call GetAllVotingPowers()
 *     to produce the snapshot input (each eligible address maps to 1).
 *  3. CastVote() uses the snapshot power (0 or 1) for that specific proposal.
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
     * Only addresses with amount > 0 are recorded.
     */
    void SetBalances(const std::map<std::vector<uint8_t>, uint64_t>& balances);

    /**
     * Update or insert the balance for a single address.
     * Setting amount to 0 removes the address (no longer a holder).
     */
    void UpdateBalance(const std::vector<uint8_t>& address, uint64_t amount);

    // ------------------------------------------------------------------ //
    //  Voting-power queries (same interface as StakingRegistry)            //
    // ------------------------------------------------------------------ //

    /**
     * Returns 1 if the address holds any tokens, 0 otherwise.
     * (One-address-one-vote: balance size is irrelevant.)
     */
    uint64_t GetVotingPower(const std::vector<uint8_t>& address) const;

    /**
     * Returns the total number of eligible voters (addresses with balance > 0).
     * This equals the maximum possible YES + NO + ABSTAIN + VETO vote count.
     */
    uint64_t GetTotalVotingPower() const;

    /**
     * Snapshot-ready vector of (address, 1) pairs for every holder.
     * Each entry has voting_power == 1.
     */
    std::vector<std::pair<std::vector<uint8_t>, uint64_t>> GetAllVotingPowers() const;

    /**
     * Number of tracked addresses (= number of eligible voters).
     */
    size_t Size() const { return balances_.size(); }

  private:
    // address → token balance (kept for holder-membership check only;
    // the actual voting power returned is always 1, never the raw amount).
    std::map<std::vector<uint8_t>, uint64_t> balances_;
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_BALANCE_VOTING_H
