#ifndef PARTHENON_GOVERNANCE_SNAPSHOT_H
#define PARTHENON_GOVERNANCE_SNAPSHOT_H

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace parthenon {
namespace governance {

/**
 * SnapshotRegistry
 *
 * Implements snapshot voting: when a proposal enters its ACTIVE state
 * (at voting_start block), the current voting power of every participant
 * is frozen into an immutable snapshot.  All votes cast on that proposal
 * use the snapshot power, regardless of how their stake changes later.
 *
 * Why this matters
 * ----------------
 *  Without snapshots, a user can acquire tokens the block before the
 *  voting window opens, cast a large vote, and immediately unstake.
 *  This is a "last-block attack" that disproportionately amplifies
 *  transient holders.  Snapshot voting eliminates this attack surface.
 *
 * Usage
 * -----
 *  1. When a proposal becomes ACTIVE, call CreateSnapshot() with the
 *     current address→power mapping from StakingRegistry.
 *  2. When CastVote() is called, call GetSnapshotPower() to get the
 *     frozen power for that voter.  If the voter has no snapshot entry
 *     (joined after the snapshot), their power is 0 for that proposal.
 */
class SnapshotRegistry {
  public:
    struct SnapshotEntry {
        std::vector<uint8_t> address;
        uint64_t             voting_power;
    };

    struct Snapshot {
        uint64_t                   proposal_id;
        uint64_t                   block_height;    // block when snapshot was taken
        uint64_t                   total_power;     // sum of all voting_power values
        std::vector<SnapshotEntry> entries;
    };

    // ------------------------------------------------------------------ //
    //  Creating a snapshot                                                 //
    // ------------------------------------------------------------------ //

    /**
     * Create a snapshot for `proposal_id` at `block_height`.
     *
     * powers – vector of (address, voting_power) pairs taken from
     *          StakingRegistry::GetVotingPower() at block_height.
     *
     * Returns false if a snapshot for this proposal already exists.
     */
    bool CreateSnapshot(uint64_t proposal_id, uint64_t block_height,
                        const std::vector<std::pair<std::vector<uint8_t>,
                                                    uint64_t>>& powers);

    // ------------------------------------------------------------------ //
    //  Querying a snapshot                                                 //
    // ------------------------------------------------------------------ //

    /**
     * Returns the frozen voting power for `address` in `proposal_id`'s
     * snapshot.  Returns 0 if the address has no entry (e.g. staked after
     * the snapshot block).
     */
    uint64_t GetSnapshotPower(uint64_t proposal_id,
                              const std::vector<uint8_t>& address) const;

    bool     HasSnapshot(uint64_t proposal_id) const;
    uint64_t GetSnapshotBlock(uint64_t proposal_id) const;
    uint64_t GetSnapshotTotalPower(uint64_t proposal_id) const;

    std::optional<Snapshot> GetSnapshot(uint64_t proposal_id) const;

    size_t SnapshotCount() const { return snapshots_.size(); }

  private:
    // proposal_id → Snapshot
    std::map<uint64_t, Snapshot> snapshots_;
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_SNAPSHOT_H
