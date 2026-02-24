#ifndef PARTHENON_GOVERNANCE_STAKING_H
#define PARTHENON_GOVERNANCE_STAKING_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace parthenon {
namespace governance {

/**
 * StakingRegistry
 *
 * Governance-layer staking: token holders lock tokens to gain
 * on-chain voting power.  This connects raw token ownership to the
 * voting_power argument passed to VotingSystem::CastVote().
 *
 * Ancient-Greece analogy
 * ----------------------
 *  Timocracy  – Solon's Athenian system allocated political rights
 *               by property/wealth tier.  Here stake replaces property,
 *               and AntiWhaleGuard limits plutocratic dominance.
 *
 *  Ateleia    – Tax exemption / reward for public service.
 *               StakingRewards distributes inflationary rewards to
 *               long-term stakers.
 *
 * Lifecycle
 * ---------
 *  Stake()           – lock tokens; creates or increases a stake record.
 *  RequestUnstake()  – start the cooldown; tokens are locked for
 *                      unstake_cooldown_blocks before they can be claimed.
 *  ClaimUnstake()    – transfer tokens back after cooldown expires.
 *  Slash()           – reduce stake and record the infraction (e.g. for
 *                      governance attacks or Byzantine behaviour).
 *
 * Voting power
 * ------------
 *  GetVotingPower() returns the stake amount for addresses with NO active
 *  pending unstake; partial unstakes reduce voting power proportionally.
 */
class StakingRegistry {
  public:
    // ------------------------------------------------------------------ //
    //  Data types                                                          //
    // ------------------------------------------------------------------ //

    struct StakeRecord {
        std::vector<uint8_t> address;
        uint64_t             staked_amount;
        uint64_t             locked_until_block;  // 0 = no extra lock
        uint64_t             staked_at_block;
        uint64_t             pending_unstake;      // amount in cooldown
    };

    enum class UnstakeStatus { PENDING, CLAIMABLE, CLAIMED };

    struct UnstakeRequest {
        std::vector<uint8_t> address;
        uint64_t             amount;
        uint64_t             requested_at_block;
        uint64_t             claimable_at_block;
        UnstakeStatus        status;
    };

    struct SlashRecord {
        std::vector<uint8_t> address;
        uint64_t             amount;
        std::string          reason;
        uint64_t             block_height;
    };

    // ------------------------------------------------------------------ //
    //  Construction                                                        //
    // ------------------------------------------------------------------ //

    /**
     * unstake_cooldown_blocks – blocks between RequestUnstake and
     *                           ClaimUnstake (prevents flash-staking).
     */
    explicit StakingRegistry(uint64_t unstake_cooldown_blocks = 50400);

    // ------------------------------------------------------------------ //
    //  Staking                                                             //
    // ------------------------------------------------------------------ //

    /**
     * Lock `amount` tokens for address.  lock_period is an extra block
     * count on top of the normal unstake cooldown during which the tokens
     * cannot even begin unstaking (e.g. for council-term commitments).
     * Pass 0 for no extra lock.
     */
    bool Stake(const std::vector<uint8_t>& address, uint64_t amount,
               uint64_t lock_period, uint64_t block_height);

    /**
     * Begin unstaking `amount` tokens.  Fails if:
     *  - address has no stake record
     *  - amount > staked_amount − pending_unstake
     *  - stake is locked (locked_until_block > block_height)
     */
    bool RequestUnstake(const std::vector<uint8_t>& address,
                        uint64_t amount, uint64_t block_height);

    /**
     * Complete a pending unstake request once cooldown has elapsed.
     * Returns false if claimable_at_block has not yet been reached.
     */
    bool ClaimUnstake(const std::vector<uint8_t>& address,
                      uint64_t block_height);

    /**
     * Slash `amount` from address's stake (e.g. for governance attack).
     * Records the infraction in the slash history.
     */
    bool Slash(const std::vector<uint8_t>& address, uint64_t amount,
               const std::string& reason, uint64_t block_height);

    // ------------------------------------------------------------------ //
    //  Queries                                                             //
    // ------------------------------------------------------------------ //

    /**
     * Returns current staked amount (including pending unstake).
     */
    uint64_t GetStake(const std::vector<uint8_t>& address) const;

    /**
     * Returns effective voting power = staked_amount − pending_unstake.
     * Tokens in cooldown do not confer voting rights.
     */
    uint64_t GetVotingPower(const std::vector<uint8_t>& address) const;

    bool IsStakeLocked(const std::vector<uint8_t>& address,
                       uint64_t block_height) const;

    std::optional<StakeRecord>   GetStakeRecord(const std::vector<uint8_t>& address) const;
    std::optional<UnstakeRequest> GetUnstakeRequest(const std::vector<uint8_t>& address) const;

    /**
     * Sum of all staked tokens (useful as total_supply proxy for anti-whale).
     */
    uint64_t GetTotalStaked() const;

    /**
     * Sum of all voting-power-eligible tokens.
     */
    uint64_t GetTotalVotingPower() const;

    /**
     * Returns a snapshot-ready vector of (address, voting_power) pairs
     * for every staker with non-zero voting power.
     */
    std::vector<std::pair<std::vector<uint8_t>, uint64_t>> GetAllVotingPowers() const;

    const std::vector<SlashRecord>& GetSlashHistory() const;

    // ------------------------------------------------------------------ //
    //  Configuration                                                       //
    // ------------------------------------------------------------------ //

    void     SetCooldown(uint64_t blocks) { unstake_cooldown_ = blocks; }
    uint64_t GetCooldown() const          { return unstake_cooldown_; }

  private:
    uint64_t unstake_cooldown_;

    std::map<std::vector<uint8_t>, StakeRecord>    stakes_;
    std::map<std::vector<uint8_t>, UnstakeRequest> unstake_requests_;
    std::vector<SlashRecord>                        slash_history_;
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_STAKING_H
