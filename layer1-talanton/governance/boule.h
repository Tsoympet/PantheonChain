#ifndef PARTHENON_GOVERNANCE_BOULE_H
#define PARTHENON_GOVERNANCE_BOULE_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace parthenon {
namespace governance {

/**
 * Boule  –  The Athenian Council of 500, adapted for blockchain governance.
 *
 * In ancient Athens the Boule was a citizen council chosen by sortition
 * (lot) rather than election, ensuring no single faction could seize
 * permanent control.  Its blockchain analogue fulfils the same role:
 *
 *  Kleroterion  (sortition)
 *      Citizens register; at each epoch the Boule is drawn by
 *      deterministic pseudo-random selection from the registered pool
 *      using a seed (e.g. the epoch boundary block hash).
 *
 *  Dokimasia  (eligibility screening)
 *      Only citizens that have registered with a minimum stake can be
 *      selected.  min_stake_to_register defaults to 0 (open access) and
 *      is configurable.
 *
 *  Proposal screening
 *      Before a proposal reaches the full Ekklesia (assembly vote) it
 *      must receive Boule approval: 2/3 of council members must vote
 *      FOR the proposal.  Boule screening can be globally toggled; when
 *      disabled the Boule still exists but screening is bypassed.
 *
 *  Graphe Paranomon  (unconstitutionality challenge)
 *      Any council member may raise a Graphe Paranomon against a
 *      proposal they believe is unconstitutional.  Once raised, the
 *      proposal is paused until the challenge is resolved by a council
 *      super-majority or dismissed.
 *
 *  Prytany  (rotating executive committee)
 *      The first (council_size / 10) members (by selection order) form
 *      the Prytany for the current epoch and can fast-track emergency
 *      proposals.
 */
class Boule {
  public:
    // ------------------------------------------------------------------ //
    //  Data types                                                          //
    // ------------------------------------------------------------------ //

    struct Citizen {
        std::vector<uint8_t> address;
        uint64_t             registered_at_block;
        uint64_t             stake_amount;  // Dokimasia: stake at registration
        bool                 is_eligible;   // false if slashed / ostracised
    };

    struct CouncilMember {
        std::vector<uint8_t> address;
        uint64_t             selected_at_block;
        uint64_t             term_end_block;
        uint32_t             selection_index;  // position in this epoch's draw
    };

    struct ProposalReview {
        uint64_t             proposal_id;
        std::vector<uint8_t> reviewer;
        bool                 approved;      // true = approve, false = reject
        std::string          rationale;
        uint64_t             reviewed_at_block;
    };

    struct GrapheParanomon {
        uint64_t             proposal_id;
        std::vector<uint8_t> challenger;
        std::string          grounds;       // reason for challenge
        uint64_t             raised_at_block;
        bool                 resolved;
        bool                 upheld;        // true = challenge upheld (proposal blocked)
        uint64_t             dismiss_votes; // council votes to dismiss
        uint64_t             uphold_votes;  // council votes to uphold
    };

    // ------------------------------------------------------------------ //
    //  Construction                                                        //
    // ------------------------------------------------------------------ //

    /**
     * council_size        – target size of the council (e.g. 21).
     * term_blocks         – how many blocks one council term lasts.
     * min_stake           – Dokimasia: minimum stake required to register.
     * screening_required  – whether proposals must pass Boule before voting.
     */
    Boule(uint32_t council_size  = 21,
          uint64_t term_blocks   = 50400,   // ~7 days at 12-s block time
          uint64_t min_stake     = 0,
          bool     screening_required = true);

    // ------------------------------------------------------------------ //
    //  Citizen registry (Dokimasia)                                        //
    // ------------------------------------------------------------------ //

    bool RegisterCitizen(const std::vector<uint8_t>& address,
                         uint64_t stake_amount,
                         uint64_t block_height);

    bool SetCitizenEligibility(const std::vector<uint8_t>& address, bool eligible);

    bool IsCitizenRegistered(const std::vector<uint8_t>& address) const;
    bool IsCouncilMember(const std::vector<uint8_t>& address) const;

    std::vector<Citizen>       GetCitizens()       const;
    std::vector<CouncilMember> GetCurrentCouncil() const;

    // ------------------------------------------------------------------ //
    //  Kleroterion (sortition)                                             //
    // ------------------------------------------------------------------ //

    /**
     * Draw a new council from the pool of eligible registered citizens.
     *
     * seed       – 32-byte entropy (e.g. epoch boundary block hash).
     *              Must have size() >= 4.
     * block_height – current block, used to set term_end_block.
     *
     * Returns false if fewer eligible citizens than council_size.
     */
    bool ConductSortition(const std::vector<uint8_t>& seed, uint64_t block_height);

    // ------------------------------------------------------------------ //
    //  Proposal screening                                                  //
    // ------------------------------------------------------------------ //

    /**
     * A council member records their YES/NO review of a proposal.
     * Requires council_member to be in the current council.
     */
    bool ReviewProposal(uint64_t proposal_id,
                        const std::vector<uint8_t>& council_member,
                        bool approved,
                        const std::string& rationale,
                        uint64_t block_height);

    /**
     * Returns true when >= 2/3 of council have approved the proposal and
     * no outstanding Graphe Paranomon challenge blocks it.
     */
    bool IsProposalApproved(uint64_t proposal_id) const;

    std::vector<ProposalReview> GetProposalReviews(uint64_t proposal_id) const;

    // ------------------------------------------------------------------ //
    //  Graphe Paranomon (unconstitutionality challenge)                    //
    // ------------------------------------------------------------------ //

    bool RaiseGrapheParanomon(uint64_t proposal_id,
                              const std::vector<uint8_t>& challenger,
                              const std::string& grounds,
                              uint64_t block_height);

    /**
     * Council member votes to resolve the challenge.
     * dismiss=true → vote to let the proposal proceed.
     * dismiss=false → vote to uphold the challenge (block the proposal).
     */
    bool VoteOnGrapheParanomon(uint64_t proposal_id,
                               const std::vector<uint8_t>& council_member,
                               bool dismiss,
                               uint64_t block_height);

    bool HasActiveChallenge(uint64_t proposal_id) const;
    std::optional<GrapheParanomon> GetChallenge(uint64_t proposal_id) const;

    // ------------------------------------------------------------------ //
    //  Prytany (rotating executive)                                        //
    // ------------------------------------------------------------------ //

    /**
     * Returns the Prytany for the current epoch: the first
     * (council_size / 10) members of the council by selection_index.
     * Returns empty vector when no council has been selected.
     */
    std::vector<CouncilMember> GetPrytany() const;

    bool IsPrytanyMember(const std::vector<uint8_t>& address) const;

    // ------------------------------------------------------------------ //
    //  Configuration                                                       //
    // ------------------------------------------------------------------ //

    uint32_t GetCouncilSize()        const { return council_size_; }
    uint64_t GetTermBlocks()         const { return term_blocks_; }
    uint64_t GetMinStake()           const { return min_stake_; }
    bool     IsScreeningRequired()   const { return screening_required_; }
    void     SetScreeningRequired(bool v)  { screening_required_ = v; }
    void     SetMinStake(uint64_t s)       { min_stake_ = s; }

    size_t   GetRegisteredCitizenCount() const { return citizens_.size(); }

  private:
    uint32_t council_size_;
    uint64_t term_blocks_;
    uint64_t min_stake_;
    bool     screening_required_;

    // Registry
    std::vector<Citizen>       citizens_;
    std::vector<CouncilMember> council_;

    // Reviews: proposal_id -> list of reviews
    std::map<uint64_t, std::vector<ProposalReview>> reviews_;

    // Challenges: proposal_id -> challenge record
    std::map<uint64_t, GrapheParanomon> challenges_;
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_BOULE_H
