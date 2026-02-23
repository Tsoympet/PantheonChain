#ifndef PARTHENON_GOVERNANCE_TREASURY_H
#define PARTHENON_GOVERNANCE_TREASURY_H

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace parthenon {
namespace governance {

/**
 * Treasury
 *
 * A full on-chain treasury that replaces the minimal TreasuryManager in
 * voting.h.  Key design principles (ancient-Greece analogy):
 *
 *  Hieromnemones (sacred treasury keepers)
 *      Multi-signature spending for the EMERGENCY track; all other tracks
 *      require an executed governance proposal.
 *
 *  Tamias (treasurer)
 *      Single-track spending with proposal authorisation.  Each Track has
 *      an independent balance and budget ceiling per period.
 *
 *  Theoric fund (Theōrika)
 *      The GRANTS track mirrors the Athenian Theoric fund – tokens paid
 *      out to citizens for public purposes, here via milestone grants.
 *
 * Tracks
 * ------
 *  CORE_DEVELOPMENT – protocol work, audits, infrastructure
 *  GRANTS           – community / ecosystem grants (milestone-gated)
 *  OPERATIONS       – day-to-day operational expenses
 *  EMERGENCY        – reserve; spendable only by EmergencyCouncil multi-sig
 *  UNCATEGORIZED    – catch-all for unclassified deposits
 *
 * Budget periods
 * --------------
 * A BudgetPeriod is created by governance and sets per-track spending caps
 * for a block range.  Spending within a track is blocked when the track's
 * cap is exhausted for the active period.
 *
 * Milestone grants
 * ----------------
 * A grant allocates funds in GRANTS track, releasing them to the recipient
 * one milestone at a time.  Unused milestones can be revoked by governance.
 *
 * Multi-sig spending (EMERGENCY track)
 * -------------------------------------
 * A pending spend is created by any guardian, then counter-signed by the
 * required number of additional guardians before execution.
 */
class Treasury {
  public:
    // ------------------------------------------------------------------ //
    //  Enumerations                                                        //
    // ------------------------------------------------------------------ //

    enum class Track {
        CORE_DEVELOPMENT,
        GRANTS,
        OPERATIONS,
        EMERGENCY,
        UNCATEGORIZED
    };

    // ------------------------------------------------------------------ //
    //  Budget periods                                                      //
    // ------------------------------------------------------------------ //

    struct BudgetPeriod {
        uint64_t              period_id;
        uint64_t              start_block;
        uint64_t              end_block;
        std::map<Track, uint64_t> track_limits;  // 0 = unlimited
        std::map<Track, uint64_t> track_spent;
    };

    // ------------------------------------------------------------------ //
    //  Milestone grant                                                     //
    // ------------------------------------------------------------------ //

    struct Milestone {
        std::string description;
        uint64_t    amount;
        bool        released;
        uint64_t    released_at_block;

        Milestone(std::string d, uint64_t a)
            : description(std::move(d)), amount(a),
              released(false), released_at_block(0) {}
    };

    struct Grant {
        uint64_t                  grant_id;
        uint64_t                  proposal_id;
        std::vector<uint8_t>      recipient;
        std::string               purpose;
        uint64_t                  total_amount;       // sum of milestone amounts
        uint64_t                  released_amount;
        bool                      revoked;
        uint64_t                  created_at_block;
        std::vector<Milestone>    milestones;
    };

    // ------------------------------------------------------------------ //
    //  Multi-sig spend (EMERGENCY track)                                  //
    // ------------------------------------------------------------------ //

    struct MultiSigSpend {
        uint64_t                         spend_id;
        uint64_t                         amount;
        std::vector<uint8_t>             recipient;
        std::string                      purpose;
        std::vector<uint8_t>             initiator;
        uint64_t                         created_at_block;
        bool                             executed;
        uint64_t                         executed_at_block;
        std::set<std::vector<uint8_t>>   signers;   // addresses that signed
    };

    // ------------------------------------------------------------------ //
    //  Transaction record (audit log)                                     //
    // ------------------------------------------------------------------ //

    struct TxRecord {
        uint64_t             tx_id;
        bool                 is_deposit;
        uint64_t             amount;
        std::vector<uint8_t> address;
        Track                track;
        uint64_t             proposal_id;  // 0 for deposits / multi-sig
        uint64_t             grant_id;     // 0 if not a grant release
        std::string          purpose;
        uint64_t             block_height;
    };

    // ------------------------------------------------------------------ //
    //  Construction                                                        //
    // ------------------------------------------------------------------ //

    /**
     * multisig_required – minimum number of guardian signatures needed
     *                     before a MultiSigSpend can be executed.
     * reserve_ratio_bps – EMERGENCY track must hold at least this fraction
     *                     of total treasury balance (basis points). 0 = off.
     */
    explicit Treasury(uint32_t multisig_required = 2,
                      uint64_t reserve_ratio_bps = 1000);  // 10 %

    // ------------------------------------------------------------------ //
    //  Guardians (multi-sig authorisation for EMERGENCY track)            //
    // ------------------------------------------------------------------ //

    bool AddGuardian(const std::vector<uint8_t>& address);
    bool RemoveGuardian(const std::vector<uint8_t>& address);
    bool IsGuardian(const std::vector<uint8_t>& address) const;
    std::vector<std::vector<uint8_t>> GetGuardians() const;

    // ------------------------------------------------------------------ //
    //  Deposits                                                            //
    // ------------------------------------------------------------------ //

    bool Deposit(uint64_t amount, const std::vector<uint8_t>& from,
                 Track track, uint64_t block_height);

    // ------------------------------------------------------------------ //
    //  Single-track spending (requires governance proposal)               //
    // ------------------------------------------------------------------ //

    /**
     * Spend from any non-EMERGENCY track.  Fails if:
     *  - proposal_id == 0
     *  - amount > track balance
     *  - active budget period is exhausted for this track
     *  - spending would breach the reserve ratio
     */
    bool Spend(uint64_t amount, const std::vector<uint8_t>& to,
               uint64_t proposal_id, Track track,
               const std::string& purpose, uint64_t block_height);

    // ------------------------------------------------------------------ //
    //  Multi-sig spending (EMERGENCY track only)                          //
    // ------------------------------------------------------------------ //

    uint64_t ProposeMultiSigSpend(uint64_t amount,
                                  const std::vector<uint8_t>& to,
                                  const std::string& purpose,
                                  const std::vector<uint8_t>& initiator,
                                  uint64_t block_height);

    bool SignMultiSigSpend(uint64_t spend_id,
                           const std::vector<uint8_t>& guardian);

    bool ExecuteMultiSigSpend(uint64_t spend_id, uint64_t block_height);

    bool HasSufficientSignatures(uint64_t spend_id) const;

    std::optional<MultiSigSpend> GetMultiSigSpend(uint64_t spend_id) const;

    // ------------------------------------------------------------------ //
    //  Budget periods                                                      //
    // ------------------------------------------------------------------ //

    uint64_t CreateBudgetPeriod(uint64_t start_block, uint64_t end_block,
                                const std::map<Track, uint64_t>& limits,
                                uint64_t proposal_id);

    /**
     * Returns true when the current period still has headroom for the
     * given (track, amount) combination.  True when no period is active
     * or the track has no limit set.
     */
    bool IsWithinBudget(Track track, uint64_t amount,
                        uint64_t block_height) const;

    std::optional<BudgetPeriod> GetActiveBudgetPeriod(uint64_t block_height) const;

    // ------------------------------------------------------------------ //
    //  Milestone grants                                                    //
    // ------------------------------------------------------------------ //

    /**
     * Create a GRANTS-track grant with milestone-based release.
     * milestones – list of (description, amount) pairs.
     * Fails if total milestone amounts > GRANTS track balance.
     */
    uint64_t CreateGrant(uint64_t proposal_id,
                         const std::vector<uint8_t>& recipient,
                         const std::string& purpose,
                         const std::vector<std::pair<std::string, uint64_t>>& milestones,
                         uint64_t block_height);

    /**
     * Release the next unreleased milestone to the recipient.
     * Requires governance proposal_id (milestone approval).
     */
    bool ReleaseMilestone(uint64_t grant_id, uint32_t milestone_index,
                          uint64_t proposal_id, uint64_t block_height);

    /**
     * Revoke remaining unreleased milestones and return funds to the
     * GRANTS track.
     */
    bool RevokeGrant(uint64_t grant_id, uint64_t proposal_id,
                     uint64_t block_height);

    std::optional<Grant> GetGrant(uint64_t grant_id) const;

    // ------------------------------------------------------------------ //
    //  Balance queries                                                     //
    // ------------------------------------------------------------------ //

    uint64_t GetTotalBalance() const;
    uint64_t GetTrackBalance(Track track) const;
    uint64_t GetReserveBalance() const;  // EMERGENCY track balance

    // ------------------------------------------------------------------ //
    //  Audit log                                                           //
    // ------------------------------------------------------------------ //

    const std::vector<TxRecord>& GetTransactions() const;

    std::vector<TxRecord> GetTransactionsByTrack(Track track) const;

    // ------------------------------------------------------------------ //
    //  Configuration                                                       //
    // ------------------------------------------------------------------ //

    void     SetMultiSigRequired(uint32_t n) { multisig_required_ = n; }
    uint32_t GetMultiSigRequired() const     { return multisig_required_; }
    void     SetReserveRatioBps(uint64_t bps) { reserve_ratio_bps_ = bps; }
    uint64_t GetReserveRatioBps() const       { return reserve_ratio_bps_; }

  private:
    uint32_t multisig_required_;
    uint64_t reserve_ratio_bps_;

    std::map<Track, uint64_t>                   balances_;
    std::vector<std::vector<uint8_t>>           guardians_;
    std::map<uint64_t, MultiSigSpend>           multisig_spends_;
    std::vector<BudgetPeriod>                   budget_periods_;
    std::map<uint64_t, Grant>                   grants_;
    std::vector<TxRecord>                       transactions_;

    uint64_t next_spend_id_   = 1;
    uint64_t next_period_id_  = 1;
    uint64_t next_grant_id_   = 1;
    uint64_t next_tx_id_      = 1;

    void RecordTx(bool is_deposit, uint64_t amount,
                  const std::vector<uint8_t>& address,
                  Track track, uint64_t proposal_id,
                  uint64_t grant_id, const std::string& purpose,
                  uint64_t block_height);

    bool ViolatesReserve(uint64_t debit_non_emergency) const;
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_TREASURY_H
