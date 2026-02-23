#ifndef PARTHENON_GOVERNANCE_OSTRACISM_H
#define PARTHENON_GOVERNANCE_OSTRACISM_H

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace parthenon {
namespace governance {

/**
 * Ostracism
 *
 * In Athens, once per year citizens could vote to exile a person deemed
 * dangerous to the democracy for ten years—no trial, no formal charges,
 * purely a community safety valve.  On-chain, this translates to a
 * community-driven temporary ban that strips an address of governance
 * rights (voting, proposing, Boule participation) for a configurable
 * number of blocks.
 *
 * Lifecycle
 * ---------
 *  NOMINATED   – any registered address may nominate another
 *  VOTING      – once nominated, community members vote FOR or AGAINST
 *  OSTRACIZED  – when FOR votes reach required_votes, subject is banned
 *  REHABILITATED – after ban_duration_blocks the subject may re-enter
 *
 * Design decisions
 * ----------------
 *  • Each address can only cast one vote per nomination.
 *  • A subject that already has an active ban cannot be re-nominated.
 *  • The nominator's address is recorded for accountability.
 *  • `Finalize()` must be called explicitly to apply the ban once the
 *    vote threshold is reached (allows the caller to batch state updates).
 */
class Ostracism {
  public:
    enum class State { NOMINATED, OSTRACIZED, REHABILITATED };

    struct Record {
        std::vector<uint8_t> subject;
        std::vector<uint8_t> nominator;
        std::string          reason;
        uint64_t             nominated_at_block;
        uint64_t             ostracized_at_block;  // 0 while pending
        uint64_t             ban_end_block;         // 0 while pending
        uint64_t             votes_for;
        uint64_t             votes_against;
        State                state;
    };

    /**
     * ban_duration_blocks – how long the ban lasts once applied.
     * required_votes_for  – votes needed to trigger the ban.
     */
    explicit Ostracism(uint64_t ban_duration_blocks = 50400,  // ~7 days
                       uint64_t required_votes_for  = 10);

    // ------------------------------------------------------------------ //
    //  Nomination                                                          //
    // ------------------------------------------------------------------ //

    /**
     * Nominate `subject` for ostracism.
     * Returns false if subject is already ostracised or already nominated.
     */
    bool Nominate(const std::vector<uint8_t>& subject,
                  const std::vector<uint8_t>& nominator,
                  const std::string& reason,
                  uint64_t block_height);

    // ------------------------------------------------------------------ //
    //  Voting                                                              //
    // ------------------------------------------------------------------ //

    /**
     * Cast a vote on the nomination of `subject`.
     * vote_to_ostracize=true → FOR (ban), false → AGAINST.
     * Returns false if voter already voted, subject not nominated, etc.
     */
    bool Vote(const std::vector<uint8_t>& subject,
              const std::vector<uint8_t>& voter,
              bool vote_to_ostracize,
              uint64_t block_height);

    bool HasVoted(const std::vector<uint8_t>& subject,
                  const std::vector<uint8_t>& voter) const;

    // ------------------------------------------------------------------ //
    //  Finalisation                                                        //
    // ------------------------------------------------------------------ //

    /**
     * Apply the ban when votes_for >= required_votes_for.
     * Returns false if threshold not reached or already ostracised.
     */
    bool Finalize(const std::vector<uint8_t>& subject, uint64_t block_height);

    // ------------------------------------------------------------------ //
    //  Status queries                                                      //
    // ------------------------------------------------------------------ //

    bool IsOstracized(const std::vector<uint8_t>& address,
                      uint64_t block_height) const;

    bool IsNominated(const std::vector<uint8_t>& address) const;

    std::optional<Record> GetRecord(const std::vector<uint8_t>& address) const;

    // ------------------------------------------------------------------ //
    //  Rehabilitation                                                      //
    // ------------------------------------------------------------------ //

    /**
     * After the ban period the subject may call Rehabilitate to formally
     * restore their governance rights.
     * Returns false if ban has not yet expired.
     */
    bool Rehabilitate(const std::vector<uint8_t>& address, uint64_t block_height);

    // ------------------------------------------------------------------ //
    //  Configuration                                                       //
    // ------------------------------------------------------------------ //

    uint64_t GetBanDuration()      const { return ban_duration_blocks_; }
    uint64_t GetRequiredVotes()    const { return required_votes_for_; }
    void SetBanDuration(uint64_t v)      { ban_duration_blocks_ = v; }
    void SetRequiredVotes(uint64_t v)    { required_votes_for_ = v; }

  private:
    uint64_t ban_duration_blocks_;
    uint64_t required_votes_for_;

    // subject address → record
    std::map<std::vector<uint8_t>, Record> records_;

    // subject address → set of voter addresses
    std::map<std::vector<uint8_t>, std::set<std::vector<uint8_t>>> voters_;
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_OSTRACISM_H
