# PantheonChain Tokenomics Audit

**Audit date:** 2026-03-07
**Scope:** TALANTON (L1), DRACHMA (L2), OBOLOS (L3) – supply caps, emission schedules, reward paths, fee mechanics, cross-layer coupling.
**Repository state:** See commit history; audit addresses the state at time of writing.

---

## Executive Technical Summary

PantheonChain uses a three-token model with distinct consensus layers and a layered settlement hierarchy (OBOLOS → DRACHMA → TALANTON). This audit:

1. **Corrected two supply-constant bugs** in `asset.h` and `issuance.h`:
   - `DRM_MAX_SUPPLY` was incorrectly set to 100 billion (matching the XRP supply); correct value is **41,000,000 DRM**.
   - `OBL_MAX_SUPPLY` was incorrectly set to 100 billion; correct value is **61,000,000 OBL**.
   - `DRM_INITIAL_REWARD` was incorrectly set to 238,000/block; correct value is **97 DRM/block**.
   - `OBL_INITIAL_REWARD` was incorrectly set to 238,000/block; correct value is **145 OBL/block**.
2. **Corrected a denomination alias collision** in `denominations.cpp`: the alias `"obolos"` was erroneously registered as an alias for the DRACHMA "obol" sub-denomination, shadowing the OBOLOS token name.
3. **Added deterministic validation scripts** under `scripts/economics/`.
4. **Documented known gaps** between genesis-JSON parameters and the C++ implementation.

The arithmetic, supply caps, and reward schedules are now internally coherent. Several economic assumptions remain **underdetermined** and are flagged below.

---

## Highest-Priority Remaining Risks

The following four issues require the most immediate attention after the supply-constant bug fixes already applied in this audit. They represent either unimplemented behavior that is documented as if it were enforced, or a silent mismatch that can cause hard-to-debug failures in production.

1. **`genesis_drachma.json` / `genesis_obolos.json` declare `annual_rate` PoS inflation fields that are not enforced.**
   Both genesis files specify a fixed-rate annual inflation (5 % for DRACHMA, 7 % for OBOLOS) consistent with a PoS reward model. The C++ implementation in `issuance.h` uses a Bitcoin-style geometric halving schedule instead. The genesis fields are silently ignored at runtime. Any tooling, documentation, or economic modelling that reads these fields will produce figures that bear no relation to actual issuance. **Resolution:** Either remove the `annual_rate` fields and update documentation to describe the halving schedule, or implement a PoS fixed-rate issuance path that honours them.

2. **OBL gas denomination: `gas_pricing.h` uses Ethereum Gwei units (relative to 1 × 10¹⁸ wei) while OBL has 8 decimal places (1 OBL = 1 × 10⁸ base units).**
   `INITIAL_BASE_FEE = 1,000,000,000` in `gas_pricing.h` is a direct copy of Ethereum's 1 Gwei value, which assumes a 10¹⁸ base unit. OBL uses a 10⁸ base unit (like Bitcoin satoshis). This 10× mismatch means the effective minimum base fee is ten times lower than intended, and any gas cost printed as "Gwei" will appear ten times higher than the actual OBL amount charged. **Resolution:** Define `OBL_GWEI = 10` (10 base units = 1 × 10⁻⁷ OBL) in `gas_pricing.h` and recalibrate all gas price constants accordingly.

3. **Slashing ratios are declared in genesis JSON but are not enforced in any C++ consensus code.**
   `genesis_drachma.json` and `genesis_obolos.json` both specify `double_sign: 0.05` and `equivocation: 0.10`. There is no corresponding slashing logic in `layer2-drachma/consensus/pos_consensus.cpp` or `layer3-obolos/consensus/pos_consensus.cpp`. Validators that equivocate or double-sign will not be penalised, eliminating a primary PoS safety guarantee. **Resolution:** Implement slashing enforcement in the PoS consensus layer, or explicitly document that slashing is a future milestone and remove the genesis fields to prevent false confidence.

4. **No on-chain relayer reimbursement for checkpoint submission costs.**
   DRACHMA validators must submit `TX_L2_COMMIT` transactions on TALANTON, paying TALN fees from their own balance. OBOLOS validators must submit `TX_L3_COMMIT` transactions on DRACHMA, paying DRM fees. Neither chain defines a mechanism to reimburse or pre-fund these cross-layer fee obligations. If the cost of checkpoint submission exceeds validator block rewards at any point, relayers have a direct financial incentive to stop submitting checkpoints, breaking the settlement hierarchy. **Resolution:** Design and implement an on-chain relayer incentive: either a dedicated checkpoint-fee subsidy funded from the block subsidy pool, or a protocol-level fee rebate mechanism.

---

## Tokenomics Matrix

### TALANTON (TALN) — Layer 1 PoW Settlement

| Parameter | Value | Source |
|---|---|---|
| Role | PoW settlement, base-layer anchor | WHITEPAPER.md |
| Consensus role | PoW (SHA-256d), probabilistic finality | genesis_talanton.json |
| Fee role | L1 transaction fees paid to miners; TX_L2_COMMIT fees included | WHITEPAPER.md |
| Staking role | **None** — staking explicitly disabled | genesis_talanton.json |
| Governance role | None defined | — |
| Max supply (hard cap) | 21,000,000 TALN | asset.h: `TALN_MAX_SUPPLY` |
| Genesis supply | 0 (all via mining) | No premine in genesis JSON |
| Achievable supply | 21,000,000 TALN | `TALN_INITIAL_REWARD × HALVING_INTERVAL × 2 = 50 × 210,000 × 2` |
| Initial block reward | 50 TALN/block | issuance.h: `TALN_INITIAL_REWARD` |
| Halving interval | 210,000 blocks (~4 years at 10 min/block) | issuance.h: `HALVING_INTERVAL` |
| Block time target | 600 seconds (10 min) | Whitepaper assumption |
| Base unit | 1 talantonion = 0.00000001 TALN (8 decimals) | units.h: `TAL_BASE_UNIT` |
| Fee sinks | All to miner (no burn mechanism defined) | Assumed |
| Treasury | None | — |
| Inflation characteristics | Geometric decay: ~50% reduction every 4 years | Deterministic |
| Security dependency | Hash rate must remain economically attractive relative to TALN price | Economic assumption |
| Long-run security | Fee-only after halving ~33 (~264 years): **UNPROVEN** | Known gap |

### DRACHMA (DRM) — Layer 2 PoS/BFT Payments

| Parameter | Value | Source |
|---|---|---|
| Role | Payments, validator staking, checkpoint to L1 | WHITEPAPER.md |
| Consensus role | PoS/BFT validator set, economic finality | genesis_drachma.json |
| Fee role | L2 transaction fees paid to validators | Assumed |
| Staking role | Validator collateral (min 1,000 DRM) | genesis_drachma.json |
| Governance role | None defined | — |
| Max supply (hard cap) | 41,000,000 DRM | asset.h: `DRM_MAX_SUPPLY` |
| Genesis supply | 0 (all via block subsidy) | No premine |
| Achievable supply | 40,740,000 DRM | `97 × 210,000 × 2` |
| Initial block reward | 97 DRM/block (from DRM launch height 210,000) | issuance.h: `DRM_INITIAL_REWARD` |
| Halving interval | 210,000 blocks (same schedule as TALN) | issuance.h |
| Slashing | double_sign: 5%, equivocation: 10% | genesis_drachma.json |
| Epoch length | 120 blocks | genesis_drachma.json |
| Base unit | 1 drachmion = 0.00000001 DRM (8 decimals) | units.h: `DR_BASE_UNIT` |
| Fee sinks | Priority fee → validators; base fee sink undefined | Partial assumption |
| Treasury | None defined in code | — |
| Inflation characteristics | Geometric decay halving schedule | Deterministic |
| Long-run security | Fee-only after halving ~33: **UNPROVEN** | Known gap |
| **DISCREPANCY** | genesis_drachma.json declares `annual_rate: 0.05` but code implements PoW-style halving; the genesis field is **NOT enforced** | ⚠️ Gap |

### OBOLOS (OBL) — Layer 3 PoS/BFT EVM Execution

| Parameter | Value | Source |
|---|---|---|
| Role | EVM execution, gas token, validator staking, governance | WHITEPAPER.md |
| Consensus role | PoS/BFT validator set, economic finality | genesis_obolos.json |
| Fee role | Gas (EIP-1559): base fee burned, priority fee to validators | gas_pricing.h |
| Staking role | Validator collateral (min 500 OBL) | genesis_obolos.json |
| Governance role | On-chain governance (antiwhale, voting, supply policy) | layer1-talanton/governance/ |
| Max supply (hard cap) | 61,000,000 OBL | asset.h: `OBL_MAX_SUPPLY` |
| Genesis supply | 0 (all via block subsidy) | No premine |
| Achievable supply | 60,900,000 OBL | `145 × 210,000 × 2` |
| Initial block reward | 145 OBL/block (from OBL launch height 420,000) | issuance.h: `OBL_INITIAL_REWARD` |
| Halving interval | 210,000 blocks | issuance.h |
| Slashing | double_sign: 5%, equivocation: 10% | genesis_obolos.json |
| Epoch length | 60 blocks | genesis_obolos.json |
| Gas target/limit | 15M / 30M gas per block (EIP-1559) | gas_pricing.h |
| Min base fee | 7 wei | gas_pricing.h: `MIN_BASE_FEE` |
| Initial base fee | 1 Gwei (1e9 wei) | gas_pricing.h: `INITIAL_BASE_FEE` |
| Base unit | 1 obolion = 0.00000001 OBL (8 decimals) | units.h: `OB_BASE_UNIT` |
| **DISCREPANCY** | genesis_obolos.json declares `annual_rate: 0.07` but code uses halving | ⚠️ Gap |
| **DISCREPANCY** | gas_pricing.h uses Ethereum Gwei (1e9, relative to 1e18 wei), but OBL has 8 decimals (1e8 base unit) — reconciliation needed | ⚠️ Gap |

---

## TALANTON PoW Security Economics

### Reward Schedule

| Epoch | Height | Reward (TALN/block) | Epoch Supply | Cumulative |
|---|---|---|---|---|
| 0 | 0 | 50 | 10,500,000 | 10,500,000 |
| 1 | 210,000 | 25 | 5,250,000 | 15,750,000 |
| 2 | 420,000 | 12 | 2,520,000 | 18,270,000 |
| 3 | 630,000 | 6 | 1,260,000 | 19,530,000 |
| … | … | … | … | → 21,000,000 |
| 33 | 6,930,000 | 0 | 0 | ~20,999,999 |

### Findings

- **Coinbase rules:** Block reward derived from `issuance.h::GetBlockReward`; `IsValidBlockReward` enforces cap. Coherent.
- **Halving math:** Integer right-shift is deterministic. Off-by-one rounding reduces actual achievable supply by ~1 base unit vs the formula ceiling. This is expected behavior.
- **Max supply overshoot:** IMPOSSIBLE given current code. `CalculateSupplyAtHeight` caps overflow at `GetMaxSupply`. Supply approaches but never reaches 21M.
- **Long-run PoW security:** After halving epoch 33 (~264 years), block subsidy = 0. Security relies entirely on transaction fees. This is a **known long-term assumption** common to Bitcoin-style chains. Whether fee volume will be sufficient is **undetermined and depends on adoption**.
- **No explicit commit-fee mechanism:** TX_L2_COMMIT transactions (DRACHMA checkpoints to TALANTON) must compete in the mempool like any L1 transaction. There is no reserved fee path or priority mechanism for commitments.

**Classification: coherent as implemented** ✓

---

## DRACHMA Validator and Payment-Layer Economics

### Reward Schedule

| Epoch | TALN Height | DRM Reward/block | DRM Epoch Supply |
|---|---|---|---|
| 0 (DRM launch) | 210,000 | 97 | 20,370,000 |
| 1 | 420,000 | 48 | 10,080,000 |
| 2 | 630,000 | 24 | 5,040,000 |
| … | … | … | → 40,740,000 |

### Findings

- **Minimum stake vs supply:** 1,000 DRM minimum per validator. Total minimum stake for 4 validators = 4,000 DRM = 0.0098% of max supply. Entry cost is extremely low — Sybil resistance depends on incentive alignment and slashing, not economic barrier to entry.
- **Validator incentive sustainability:** Block subsidy provides primary validator income for early epochs. Long-run depends on L2 fee volume.
- **Staggered start height:** DRACHMA issuance starts at TALN-equivalent height 210,000. On the DRACHMA chain itself this is height 0. The `issuance.h` logic maps the asset-specific block counter offset correctly.
- **Genesis PoS annual_rate field:** `genesis_drachma.json` declares `"annual_rate": 0.05`. This field is **not enforced** by any current C++ code. The halving schedule is what's actually implemented. This is a documentation/implementation gap.
- **No base fee burn defined:** DRACHMA does not implement EIP-1559. Fee routing to validators is assumed but not explicitly coded.
- **Checkpoint economics:** Submitting TX_L3_COMMIT (receiving OBOLOS checkpoint) is part of validator duties. No direct fee mechanism compensates the submitting validator for the cost differential vs. other transactions.

**Classification: coherent in principle but incomplete in code** ⚠️

---

## OBOLOS Execution-Layer Economics

### Reward Schedule

| Epoch | TALN Height | OBL Reward/block | OBL Epoch Supply |
|---|---|---|---|
| 0 (OBL launch) | 420,000 | 145 | 30,450,000 |
| 1 | 630,000 | 72 | 15,120,000 |
| 2 | 840,000 | 36 | 7,560,000 |
| … | … | … | → 60,900,000 |

### Findings

- **Gas pricing:** EIP-1559 implemented in `gas_pricing.h`. Base fee burned, priority fee to validator. Coherent in structure.
- **Gas denomination mismatch:** `gas_pricing.h` uses Ethereum Gwei (1e9 base units relative to 1e18 wei), but OBL has 8 decimals (1e8 base unit). The minimum base fee of 7 wei means 7/1e18 ETH-equivalent — effectively zero if OBL is treated as 1e18-unit token. **If OBL uses 1e8 base units, gas accounting must be calibrated accordingly.** Recommend an explicit `OBL_GWEI = 10` constant (10 base units = 1e-7 OBL).
- **Token function overload:** OBL serves as gas + staking collateral + governance token. This is common (Ethereum ETH) but concentrates multiple demand drivers in one token, creating instability if any one function becomes dominant or collapses.
- **Min stake:** 500 OBL. Very accessible. Same Sybil concern as DRM.
- **Governance treasury cap:** `supply_policy.h` enforces 50% of achievable supply ceiling. No uncapped governance mint path exists in current code.
- **Genesis PoS annual_rate=0.07:** Same gap as DRACHMA — not enforced in code.

**Classification: coherent in principle but incomplete in code** ⚠️

---

## Cross-Layer Incentive Coupling

### Token Interaction

```
 TALANTON (21M)
     ↑  anchored by TX_L2_COMMIT
 DRACHMA (41M)
     ↑  anchored by TX_L3_COMMIT
 OBOLOS (61M)
```

- **Security value flow:** TALANTON's PoW security is independent of DRM/OBL token prices. DRACHMA and OBOLOS security depends on their own staking economics.
- **Checkpoint cost coupling:** DRACHMA validators pay TALN fees for TX_L2_COMMIT; OBOLOS validators pay DRM fees for TX_L3_COMMIT. If TALN price rises significantly vs DRM, relayer economics can become unfavorable.
- **Liquidity fragmentation:** Three separate native tokens require three separate liquidity pools. Early-stage liquidity fragmentation is a known bootstrapping risk.
- **Cannibalization risk:** DRM and OBL compete with TALN for user/validator capital allocation. This is intentional (separate utility domains) but creates demand fragmentation.

### Findings per Category

| Coupling | Assessment | Detail |
|---|---|---|
| TALN PoW security vs upper layers | **healthy separation** | TALANTON PoW is independent of DRM/OBL economics |
| Checkpoint fee coupling | **manageable but risky** | DRACHMA relayers pay TALN fees; no reimbursement mechanism |
| Liquidity fragmentation | **manageable but risky** | Three tokens require three separate liquid markets for healthy operation |
| Upper-layer collapse effect on TALN | **manageable** | If DRM/OBL fail, TALN continues as standalone PoW chain. Checkpoint transactions would cease but L1 operates independently |
| TALN price crash on upper layers | **dangerously coupled** | If TALN price drops severely, PoW security weakens, reducing confidence in the entire settlement hierarchy |
| Governance treasury concentration | **healthy separation** | supply_policy.h caps treasury at 50% of achievable supply per token |

---

## Supply Cap Findings

| Token | Hard Cap | Achievable | Cumulative at halving 64 | Cap Exceeded? |
|---|---|---|---|---|
| TALN | 21,000,000 | 21,000,000 | ~20,999,999 | NO ✓ |
| DRM | 41,000,000 | 40,740,000 | ~40,739,999 | NO ✓ |
| OBL | 61,000,000 | 60,900,000 | ~60,899,999 | NO ✓ |

**All supply caps are mathematically coherent.** The ~1 base unit discrepancy between the formula ceiling and simulation result is expected integer rounding from right-shift halving.

---

## Reward Schedule Coherence

| Property | TALN | DRM | OBL |
|---|---|---|---|
| Initial reward defined | 50 TALN ✓ | 97 DRM ✓ | 145 OBL ✓ |
| Reward decreases over time | ✓ | ✓ | ✓ |
| No negative rewards | ✓ | ✓ | ✓ |
| Supply cap respected | ✓ | ✓ | ✓ |
| No overflow in epoch arithmetic | ✓ | ✓ | ✓ |
| Reward zeroes after halving 64 | ✓ | ✓ | ✓ |

**All reward schedules are mathematically coherent.**

---

## Miner/Validator Incentive Sustainability

| Layer | Near-term | Long-term |
|---|---|---|
| TALN (PoW miners) | Viable — 50 TALN/block initial subsidy is substantial | **UNPROVEN** — fee-only security post-halving 33 |
| DRM (PoS validators) | Viable — 97 DRM/block subsidy covers operational cost | **UNPROVEN** — fee dependency long-term |
| OBL (PoS validators) | Viable — 145 OBL/block; EIP-1559 burn/tip adds fee dimension | **UNPROVEN** — fee + gas burn dependency long-term |

---

## Implemented vs Assumed vs Missing

### Enforced in Code

- Supply caps: `AssetSupply::GetMaxSupply()`, `IsValidBlockReward()` — **enforced**
- Issuance schedule: `Issuance::GetBlockReward()`, `CalculateSupplyAtHeight()` — **enforced**
- Governance treasury cap (50%): `SupplyPolicy::ExceedsTreasuryCap()` — **enforced**
- Validator bonding health check: `SupplyPolicy::IsBondingHealthy()` — **enforced**
- EIP-1559 gas pricing: `gas_pricing.h` functions — **enforced** (for OBOLOS)
- Denomination conversions: `units.h` with overflow guards — **enforced**
- Slashing ratios: declared in genesis JSON — **declared, not enforced in C++ consensus code**

### Exists Only in Config / Genesis

- `genesis_drachma.json: "annual_rate": 0.05` — **not enforced in issuance.h**
- `genesis_obolos.json: "annual_rate": 0.07` — **not enforced in issuance.h**
- Minimum stake values (1000 DRM, 500 OBL) — **declared, not enforced in consensus code**
- Slashing ratios (5%/10%) — **declared, not enforced in consensus code**

### Exists Only in Documentation

- Long-run fee-security model for PoW TALN
- Long-run validator incentive sustainability for DRM and OBL
- Relayer economic model (who pays checkpoint fees and why it's sustainable)
- Cross-chain bridge economics (deposit/withdrawal fee structure)

### Undefined / Missing

- On-chain enforcement of minimum stake in PoS consensus
- Slashing implementation in consensus code
- Relayer incentive / reimbursement mechanism for checkpoint submission
- Fee routing to validator/treasury for DRACHMA layer
- Gas denomination calibration for OBL (1e8 vs Ethereum 1e18 reconciliation)
- Detailed unlock/vesting schedules (no premine, so this is N/A currently)

---

## Concrete Risk List

| Priority | Risk | Classification |
|---|---|---|
| **HIGH** | Long-run TALN PoW fee-only security (halving ~33, ~264 years) | Known economic assumption |
| **HIGH** | Checkpoint relayer cost coverage: no reimbursement for TX_L2_COMMIT TALN fees | Implementation gap |
| **MEDIUM** | genesis annual_rate fields (5%/7%) not enforced — documents describe PoS inflation that isn't implemented | Documentation/code gap |
| **MEDIUM** | OBL gas denomination: Gwei units in `gas_pricing.h` vs 8-decimal OBL base unit | Implementation risk |
| **MEDIUM** | Very low minimum stake (1,000 DRM / 500 OBL) reduces Sybil resistance | Economic design assumption |
| **MEDIUM** | Slashing ratios declared but not enforced in consensus code | Implementation gap |
| **LOW** | Three-token liquidity fragmentation at bootstrap | Market structure risk |
| **LOW** | TALN price crash cascading confidence loss through settlement hierarchy | Economic coupling risk |

---

## Recommended Parameter Fixes

### Fix Immediately (code bugs — already fixed in this audit)

1. ✅ `asset.h`: `DRM_MAX_SUPPLY = 100B → 41,000,000 * BASE_UNIT`
2. ✅ `asset.h`: `OBL_MAX_SUPPLY = 100B → 61,000,000 * BASE_UNIT`
3. ✅ `asset.h`: `DRM_ACHIEVABLE_SUPPLY` / `OBL_ACHIEVABLE_SUPPLY` corrected
4. ✅ `issuance.h`: `DRM_INITIAL_REWARD = 238,000 → 97 * BASE_UNIT`
5. ✅ `issuance.h`: `OBL_INITIAL_REWARD = 238,000 → 145 * BASE_UNIT`
6. ✅ `denominations.cpp`: Removed `"obolos"` alias from DRACHMA obol sub-denomination

### Improve Next (important but not blocking)

7. Resolve genesis JSON `annual_rate` fields: either remove them (they aren't enforced) or implement a PoS fixed-rate issuance path for DRACHMA and OBOLOS to match whitepaper intent.
8. Add an `OBL_GWEI` constant in `gas_pricing.h` or a new header: `constexpr uint64_t OBL_GWEI = 10; // 10 base units = 1e-7 OBL (8-decimal equivalent of Gwei)`.
9. Implement minimum stake enforcement in PoS consensus code.
10. Implement slashing in consensus code (currently declared in genesis only).
11. Add a relayer reimbursement / fee-routing mechanism for TX_L2_COMMIT and TX_L3_COMMIT.

### Future Research

12. Model long-run fee market sufficiency for post-subsidy security (years 264+).
13. Analyze cross-chain checkpoint economics under variable TALN/DRM price ratios.
14. Model minimum bonded stake as a percentage of circulating supply, not absolute amount, as circulating supply grows.
15. Consider raising minimum stake thresholds or implementing a percentage-of-supply floor to improve Sybil resistance.
16. Formal specification of gas unit calibration for OBL (8-decimal base vs Ethereum 18-decimal convention).

---

## Validation Scripts

| Script | Purpose | Exit Code |
|---|---|---|
| `scripts/economics/check_supply_caps.py` | Verify supply caps are never exceeded | 0 = pass, 1 = fail |
| `scripts/economics/check_emission_schedule.py` | Verify halving schedule math | 0 = pass, 1 = fail |
| `scripts/economics/check_reward_paths.py` | Verify reward/fee paths per layer | 0 = pass, 1 = fail |
| `scripts/economics/check_decimal_consistency.py` | Verify denomination/unit coherence | 0 = pass, 1 = fail |
| `scripts/economics/stress_test_tokenomics.py` | Parameterized economic stress scenarios | 0 = no failures |

Run all checks:
```bash
python3 scripts/economics/check_supply_caps.py
python3 scripts/economics/check_emission_schedule.py
python3 scripts/economics/check_reward_paths.py
python3 scripts/economics/check_decimal_consistency.py
python3 scripts/economics/stress_test_tokenomics.py
```

---

*This document reflects audited state as of the commit that introduced this file. Future changes to tokenomics constants must update both the source constants and this document.*
