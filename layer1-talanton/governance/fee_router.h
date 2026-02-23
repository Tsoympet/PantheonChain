#ifndef PARTHENON_GOVERNANCE_FEE_ROUTER_H
#define PARTHENON_GOVERNANCE_FEE_ROUTER_H

#include "treasury.h"
#include "eventlog.h"

#include <cstdint>
#include <map>
#include <vector>

namespace parthenon {
namespace governance {

/**
 * FeeRouter
 *
 * Routes every fee event from the three PantheonChain layers to its
 * correct destination: block producer, treasury, or burn sink.
 *
 * How other chains handle fees (reference designs)
 * -------------------------------------------------
 *  Polkadot      80 % treasury, 20 % block author
 *  Cosmos Hub     2 % community pool, 98 % validators (+ inflation)
 *  Ethereum      base fee 100 % burned (EIP-1559), tip 100 % to validator
 *  Optimism      sequencer surplus → OP Foundation
 *  Near Protocol 70 % burned, 30 % to contract developer
 *  Cardano       20 % of staking rewards → on-chain treasury
 *
 * PantheonChain design
 * --------------------
 * Because PantheonChain has three layers with distinct economic roles,
 * each fee source has its own split, chosen to balance:
 *   • Security budget (reward block producers adequately)
 *   • Treasury sustainability (fund ongoing development)
 *   • Deflationary pressure (burn reduces long-term inflation)
 *
 *  FeeSource            Producer  Treasury  Burn   Treasury track
 *  ─────────────────────────────────────────────────────────────
 *  L1_UTXO  (TALN)       80 %     15 %      5 %   CORE_DEVELOPMENT
 *  L2_VALIDATOR (DRM)    70 %     20 %     10 %   OPERATIONS
 *  L3_BASE_FEE (OBL)      0 %     50 %     50 %   GRANTS
 *  L3_PRIORITY_FEE (OBL) 100 %     0 %      0 %   –
 *  BRIDGE_FEE             0 %    100 %      0 %   OPERATIONS
 *  PROTOCOL_FEE           0 %    100 %      0 %   UNCATEGORIZED
 *
 * Rounding: remainder after integer division is added to burn_amount
 * so that producer_amount + treasury_amount + burn_amount == total_fee
 * exactly (no satoshi leakage).
 *
 * Treasury deposit is fire-and-forget: if no Treasury is attached,
 * the treasury_amount is simply not deposited (and the split is
 * recorded in stats regardless so it can be replayed later).
 */
class FeeRouter {
  public:
    // ------------------------------------------------------------------ //
    //  Fee source taxonomy                                                 //
    // ------------------------------------------------------------------ //

    enum class FeeSource {
        L1_UTXO,          // Layer-1 UTXO transaction fees           (TALN)
        L2_VALIDATOR,     // Layer-2 PoS validator/sequencer fees    (DRM)
        L3_BASE_FEE,      // Layer-3 EVM base fee (EIP-1559 style)  (OBL)
        L3_PRIORITY_FEE,  // Layer-3 EVM priority tip                (OBL)
        BRIDGE_FEE,       // Cross-chain bridge protocol fees        (any)
        PROTOCOL_FEE,     // Miscellaneous protocol-level fees       (any)
    };

    // ------------------------------------------------------------------ //
    //  Split configuration (all values in basis points, must sum to 10000) //
    // ------------------------------------------------------------------ //

    struct SplitConfig {
        uint32_t        producer_bps;     // share to block producer
        uint32_t        treasury_bps;     // share routed to treasury
        uint32_t        burn_bps;         // share destroyed / removed from supply
        Treasury::Track treasury_track;   // which treasury track receives funds

        bool IsValid() const {
            return (static_cast<uint64_t>(producer_bps) +
                    static_cast<uint64_t>(treasury_bps)  +
                    static_cast<uint64_t>(burn_bps))      == 10000u;
        }
    };

    // ------------------------------------------------------------------ //
    //  Result of one routing call                                          //
    // ------------------------------------------------------------------ //

    struct RouteResult {
        FeeSource source;
        uint64_t  total_fee;
        uint64_t  producer_amount;   // caller should credit to block producer
        uint64_t  treasury_amount;   // deposited into treasury (if attached)
        uint64_t  burn_amount;       // removed from supply (tracked only)
        bool      treasury_deposited; // false when no treasury is attached
    };

    // ------------------------------------------------------------------ //
    //  Cumulative statistics per source                                    //
    // ------------------------------------------------------------------ //

    struct SourceStats {
        uint64_t total_fees_routed;
        uint64_t total_to_producer;
        uint64_t total_to_treasury;
        uint64_t total_burned;
        uint64_t route_count;
    };

    // ------------------------------------------------------------------ //
    //  Construction                                                        //
    // ------------------------------------------------------------------ //

    /**
     * treasury – optional; if nullptr routing still works but no deposit
     *            is made.  Attach later via SetTreasury().
     * event_log – optional audit log.
     */
    explicit FeeRouter(Treasury* treasury  = nullptr,
                       GovernanceEventLog* event_log = nullptr);

    // ------------------------------------------------------------------ //
    //  Default per-source split configurations                            //
    // ------------------------------------------------------------------ //

    static SplitConfig DefaultL1Config();
    static SplitConfig DefaultL2Config();
    static SplitConfig DefaultL3BaseFeeConfig();
    static SplitConfig DefaultL3PriorityFeeConfig();
    static SplitConfig DefaultBridgeFeeConfig();
    static SplitConfig DefaultProtocolFeeConfig();

    // ------------------------------------------------------------------ //
    //  Core routing                                                        //
    // ------------------------------------------------------------------ //

    /**
     * Route `total_fee` from `source`.
     *
     * producer_address – address to attribute producer share to
     *                    (used for event log; actual transfer done by caller
     *                    using result.producer_amount).
     * block_height     – current block (used for treasury deposit & log).
     *
     * The function:
     *   1. Splits total_fee by the configured SplitConfig.
     *   2. Deposits treasury_amount into the attached Treasury (if any).
     *   3. Records the event in the attached EventLog (if any).
     *   4. Updates internal statistics.
     *   5. Returns a RouteResult for the caller to process.
     */
    RouteResult Route(FeeSource source,
                      uint64_t total_fee,
                      const std::vector<uint8_t>& producer_address,
                      uint64_t block_height);

    // ------------------------------------------------------------------ //
    //  Configuration                                                       //
    // ------------------------------------------------------------------ //

    void SetSplitConfig(FeeSource source, const SplitConfig& cfg);
    const SplitConfig& GetSplitConfig(FeeSource source) const;

    void SetTreasury(Treasury* treasury)       { treasury_  = treasury;  }
    void SetEventLog(GovernanceEventLog* log)  { event_log_ = log;       }

    Treasury*           GetTreasury()  const { return treasury_;  }
    GovernanceEventLog* GetEventLog()  const { return event_log_; }

    // ------------------------------------------------------------------ //
    //  Statistics                                                          //
    // ------------------------------------------------------------------ //

    const SourceStats& GetSourceStats(FeeSource source) const;

    /** Aggregate across all sources. */
    SourceStats GetTotalStats() const;

    uint64_t GetTotalTreasuryRevenue() const;
    uint64_t GetTotalBurned()          const;

  private:
    Treasury*           treasury_;
    GovernanceEventLog* event_log_;

    std::map<FeeSource, SplitConfig>  configs_;
    std::map<FeeSource, SourceStats>  stats_;

    static std::string SourceName(FeeSource s);
};

}  // namespace governance
}  // namespace parthenon

#endif  // PARTHENON_GOVERNANCE_FEE_ROUTER_H
