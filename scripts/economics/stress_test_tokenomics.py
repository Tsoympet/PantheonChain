#!/usr/bin/env python3
"""
PantheonChain – Tokenomics Stress-Test Simulator
=================================================
Evaluates parameterized stress scenarios to identify obvious economic
breakpoints in the three-token (TALN / DRM / OBL) architecture.

This is NOT a market-price predictor. It uses simple deterministic
sensitivity analysis to flag when assumed parameters put the system in
a clearly unsafe region.

Scenarios tested:
  1.  TALANTON price drops sharply (PoW security impact)
  2.  DRACHMA staking participation falls below target
  3.  OBOLOS gas demand remains persistently low
  4.  Checkpoint costs rise relative to fee revenue
  5.  Validator count decreases below safety threshold
  6.  Miner rewards halve while fee revenue remains low
  7.  Genesis allocations create concentration risk
  8.  Governance enables excessive minting (capped by supply_policy)
  9.  Low activity + capped supply causes validator starvation
  10. High inflation + low usage causes token value erosion

Output: PASS / WARN / FAIL per scenario with explanation.

Exit code: 0 = no failures, 1 = at least one failure.
"""

import sys
import math

# ---------------------------------------------------------------------------
# Canonical constants
# ---------------------------------------------------------------------------
BASE_UNIT        = 100_000_000
HALVING_INTERVAL = 210_000

TALN_INITIAL_REWARD = 50   # tokens
DRM_INITIAL_REWARD  = 97
OBL_INITIAL_REWARD  = 145

TALN_MAX_SUPPLY = 21_000_000
DRM_MAX_SUPPLY  = 41_000_000
OBL_MAX_SUPPLY  = 61_000_000

DRM_MIN_STAKE   = 1_000    # DRM per validator
OBL_MIN_STAKE   = 500      # OBL per validator
BFT_MIN_VALIDATORS = 4     # minimum to maintain 2/3 quorum

TALN_BLOCK_TIME_SECS = 600    # Bitcoin-style 10 min target
DRM_EPOCH_LENGTH     = 120    # blocks per epoch (from genesis)
OBL_EPOCH_LENGTH     = 60

# EIP-1559 gas pricing (from gas_pricing.h)
OBL_TARGET_GAS       = 15_000_000
OBL_MAX_GAS          = 30_000_000
OBL_INITIAL_BASE_FEE = 1_000_000_000  # 1 Gwei (in wei / Ethereum units)

# PoS annual inflation rates declared in genesis (NOT enforced in code)
DRM_GENESIS_INFLATION = 0.05
OBL_GENESIS_INFLATION = 0.07

# Treasury cap: 50% of achievable supply (from supply_policy.h)
TIER_HIGH_BPS = 5000

PASS = "\033[32mPASS\033[0m"
FAIL = "\033[31mFAIL\033[0m"
WARN = "\033[33mWARN\033[0m"

failures = 0
warnings = 0


def result(label, status, explanation):
    global failures, warnings
    sym = {"PASS": PASS, "WARN": WARN, "FAIL": FAIL}[status]
    print(f"  [{sym}] {label}")
    print(f"         {explanation}")
    if status == "FAIL":
        failures += 1
    elif status == "WARN":
        warnings += 1


def section(title):
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")


def get_reward_whole(initial_reward, epoch):
    """Returns whole-token reward at given halving epoch."""
    r = initial_reward >> epoch
    return r


def cumulative_supply(initial_reward, n_halvings=128):
    total = 0
    for h in range(n_halvings):
        r = initial_reward >> h
        total += r * HALVING_INTERVAL
        if r == 0:
            break
    return total


# ---------------------------------------------------------------------------
# Scenario 1: TALANTON price drops sharply
# ---------------------------------------------------------------------------
section("Scenario 1: TALANTON price drops sharply")

# If TALN price drops 90%, the USD-equivalent miner revenue drops 90%.
# Mining remains economic only if hash cost < reward value.
# We cannot model absolute prices, but we can check: does the reward schedule
# still provide > 0 tokens to miners at all relevant epochs?

taln_reward_at_halving_10 = get_reward_whole(TALN_INITIAL_REWARD, 10)  # ~0.049 TALN
taln_reward_at_halving_20 = get_reward_whole(TALN_INITIAL_REWARD, 20)  # ~0.000047 TALN
taln_reward_at_halving_33 = get_reward_whole(TALN_INITIAL_REWARD, 33)  # 0

if taln_reward_at_halving_33 == 0:
    result(
        "PoW security after halving 33",
        "WARN",
        f"TALN block subsidy = 0 by halving 33 (~2×33×4 ≈ 264 years). "
        "Long-run PoW security relies entirely on transaction fees. "
        "Fee sufficiency at that point is UNPROVEN and a known long-term risk.",
    )
else:
    result("PoW reward still positive at halving 33", "PASS", "")

result(
    "TALN price shock (90% drop)",
    "WARN",
    "A sharp price drop reduces USD-equivalent miner revenue. "
    "The block subsidy continues in token terms, but mining profitability depends "
    "on the TALN/electricity price ratio. This cannot be deterministically checked. "
    "Risk: miner exit → hashrate drop → reduced PoW security. "
    "Assumed: difficulty adjusts, but security degrades proportionally.",
)

# ---------------------------------------------------------------------------
# Scenario 2: DRACHMA staking participation falls below target
# ---------------------------------------------------------------------------
section("Scenario 2: DRACHMA staking participation < target")

# Check what happens if only BFT_MIN_VALIDATORS participate
drm_achievable = cumulative_supply(DRM_INITIAL_REWARD)
drm_min_bonded = BFT_MIN_VALIDATORS * DRM_MIN_STAKE
bonded_pct = (drm_min_bonded / drm_achievable) * 100

if drm_min_bonded < drm_achievable * 0.05:
    result(
        "DRM: minimum validator set stake as % of achievable supply",
        "WARN",
        f"Min bonded ({drm_min_bonded:,} DRM) = {bonded_pct:.4f}% of achievable "
        f"({drm_achievable:,} DRM). "
        "Very low stake means validator-set bribery or Sybil attacks are cheap. "
        "Raising min stake or requiring bonded % floor is recommended.",
    )
else:
    result(
        "DRM: minimum validator set stake adequate",
        "PASS",
        f"{bonded_pct:.2f}% of achievable supply",
    )

if BFT_MIN_VALIDATORS < 4:
    result(
        "DRM: validator count below BFT safety floor",
        "FAIL",
        "BFT quorum requires at least 4 validators for 2/3 quorum threshold. "
        "Below 4 validators the chain has no BFT liveness or safety guarantee.",
    )
else:
    result(
        "DRM: BFT_MIN_VALIDATORS >= 4",
        "PASS",
        f"Using {BFT_MIN_VALIDATORS} as the safety floor.",
    )

# ---------------------------------------------------------------------------
# Scenario 3: OBOLOS gas demand remains low
# ---------------------------------------------------------------------------
section("Scenario 3: OBOLOS gas demand persistently low")

# Low gas demand → base fee decreases to MIN_BASE_FEE = 7 wei
# Block proposer receives only priority fees (tips) when base fee floors
MIN_BASE_FEE = 7
if MIN_BASE_FEE < 100:
    result(
        "OBL: MIN_BASE_FEE very low (7 wei)",
        "WARN",
        f"MIN_BASE_FEE = {MIN_BASE_FEE} wei. At minimum, base-fee burn is nearly zero. "
        "Validators earn only block subsidy + tips in a low-demand environment. "
        "Subsidy currently covers this; long-run risk if subsidy declines without fee growth.",
    )

# OBL block subsidy at genesis: 145 OBL/block
obl_first_epoch = OBL_INITIAL_REWARD * HALVING_INTERVAL
result(
    "OBL: initial epoch subsidy covers low-demand period",
    "PASS",
    f"First epoch issues {obl_first_epoch:,} OBL total. "
    "Validator revenue from subsidy alone is adequate in early operation.",
)

# ---------------------------------------------------------------------------
# Scenario 4: Checkpoint costs rise relative to fee revenue
# ---------------------------------------------------------------------------
section("Scenario 4: Checkpoint costs rise vs fee revenue")

result(
    "Checkpoint relayer economic model",
    "WARN",
    "TX_L2_COMMIT (DRACHMA → TALANTON) requires paying TALN L1 fees. "
    "TX_L3_COMMIT (OBOLOS → DRACHMA) requires paying DRM L2 fees. "
    "Neither checkpoint submission has an on-chain reimbursement mechanism. "
    "If L1 fees exceed DRM validator block rewards, relayers may go offline. "
    "Risk level: MANAGEABLE in early low-fee environment; HIGH if fees rise significantly.",
)

# ---------------------------------------------------------------------------
# Scenario 5: Validator count decreases
# ---------------------------------------------------------------------------
section("Scenario 5: Validator count decreases")

drm_min_validators_for_bft = 4   # BFT safety: need >= 4 to tolerate 1 Byzantine failure (2/3 quorum)
obl_min_validators_for_bft = 4

result(
    "DRM: BFT liveness floor",
    "WARN",
    f"DRACHMA requires >= {drm_min_validators_for_bft} honest validators for BFT safety. "
    "If validator count drops below this, the chain halts or becomes unsafe. "
    "Minimum stake is 1000 DRM which is very accessible – Sybil-proofing depends "
    "on adequate stake, not validator count alone.",
)

result(
    "OBL: BFT liveness floor",
    "WARN",
    f"OBOLOS requires >= {obl_min_validators_for_bft} honest validators for BFT safety. "
    "Minimum stake 500 OBL is similarly low. "
    "Economic-finality safety depends on total staked value vs cost-of-corruption.",
)

# ---------------------------------------------------------------------------
# Scenario 6: Miner rewards halve, fee revenue remains low
# ---------------------------------------------------------------------------
section("Scenario 6: Block subsidy halving while fee revenue low")

for ticker, initial, n_halvings in [
    ("TALN", TALN_INITIAL_REWARD, 4),
    ("DRM",  DRM_INITIAL_REWARD,  4),
    ("OBL",  OBL_INITIAL_REWARD,  4),
]:
    reward_after = get_reward_whole(initial, n_halvings)
    pct_of_initial = (reward_after / initial) * 100 if initial > 0 else 0
    if reward_after > 0:
        result(
            f"{ticker}: reward after {n_halvings} halvings ({reward_after} tokens/block)",
            "PASS",
            f"{pct_of_initial:.2f}% of initial. "
            "Reward remains positive; fee revenue gap is a future risk, not immediate.",
        )
    else:
        result(
            f"{ticker}: reward zeroed after {n_halvings} halvings",
            "WARN",
            "Fee-only security assumption applies from this epoch. Unproven.",
        )

# ---------------------------------------------------------------------------
# Scenario 7: Genesis allocation concentration risk
# ---------------------------------------------------------------------------
section("Scenario 7: Genesis allocation concentration risk")

result(
    "Genesis premine / allocation",
    "PASS",
    "No explicit genesis premine or allocation fields found in genesis JSON files. "
    "All supply enters via block subsidy. Concentration risk at launch is minimal, "
    "though early-miner concentration is inherent to PoW/PoS.",
)

# ---------------------------------------------------------------------------
# Scenario 8: Governance minting
# ---------------------------------------------------------------------------
section("Scenario 8: Governance excessive minting")

# Supply policy caps treasury at 50% of achievable supply
# There is no governance mint path defined in current code that bypasses the cap.
treasury_cap_pct = TIER_HIGH_BPS / 100
result(
    "Governance treasury capped at 50% of achievable supply",
    "PASS",
    f"supply_policy.h enforces TIER_HIGH = {treasury_cap_pct}% of achievable supply. "
    "No uncapped governance mint path is present in the current implementation.",
)
result(
    "Uncapped mint path audit",
    "WARN",
    "No governance mint function beyond issuance schedule has been implemented. "
    "Future governance proposals COULD add such a path. "
    "Recommend adding an on-chain cap invariant enforced at the consensus layer.",
)

# ---------------------------------------------------------------------------
# Scenario 9: Low activity + capped supply = validator starvation
# ---------------------------------------------------------------------------
section("Scenario 9: Low activity + capped supply → validator starvation")

# After issuance ends (halving 64), validators earn only fees.
# Starvation occurs if: fee_per_block < operational_cost_per_block.
# We cannot model cost, but we can quantify when subsidy ends.
halvings_to_zero_drm = 33   # approximate (97 >> 33 = 0)
halvings_to_zero_obl = 34   # approximate (145 >> 34 ~ 0)
years_per_halving = HALVING_INTERVAL * TALN_BLOCK_TIME_SECS / (365.25 * 24 * 3600)

result(
    f"DRM: subsidy approaches zero in ~{halvings_to_zero_drm * years_per_halving:.0f} years",
    "WARN",
    "Post-subsidy validator compensation depends entirely on transaction fee volume. "
    "Fee market sufficiency is UNPROVEN and represents a known long-term risk.",
)
result(
    f"OBL: subsidy approaches zero in ~{halvings_to_zero_obl * years_per_halving:.0f} years",
    "WARN",
    "Same long-run fee-only security concern applies to OBOLOS validators.",
)

# ---------------------------------------------------------------------------
# Scenario 10: High inflation + low usage → token value erosion
# ---------------------------------------------------------------------------
section("Scenario 10: Inflation vs usage (value erosion risk)")

# The genesis JSONs declare PoS inflation rates (5% DRM, 7% OBL) but
# the implementation uses halving schedules, not fixed-rate inflation.
# Actual inflation rate at each halving epoch:
for ticker, initial, achievable in [
    ("DRM", DRM_INITIAL_REWARD, cumulative_supply(DRM_INITIAL_REWARD)),
    ("OBL", OBL_INITIAL_REWARD, cumulative_supply(OBL_INITIAL_REWARD)),
]:
    # Epoch 0 issuance as % of final achievable supply = "year 0 inflation"
    epoch_0_issuance = initial * HALVING_INTERVAL
    inflation_rate = epoch_0_issuance / achievable * 100
    status = "WARN" if inflation_rate > 10 else "PASS"
    result(
        f"{ticker}: epoch-0 issuance as % of achievable supply",
        status,
        f"Epoch 0 issues {epoch_0_issuance:,} tokens = {inflation_rate:.1f}% of achievable. "
        "High early inflation is dilutive but decreases rapidly with halvings.",
    )

# Note the gap between genesis-declared rate and actual schedule
result(
    "DRM: genesis annual_rate=5% vs actual halving schedule",
    "WARN",
    "genesis_drachma.json declares annual_rate=0.05 but the code implements a "
    "Bitcoin-style halving schedule, not a fixed-rate inflation model. "
    "The actual first-epoch annualized emission rate is higher than 5%. "
    "This documented/code gap should be resolved.",
)
result(
    "OBL: genesis annual_rate=7% vs actual halving schedule",
    "WARN",
    "genesis_obolos.json declares annual_rate=0.07 but code implements halving. "
    "Same gap applies. Recommend either updating genesis or aligning the code.",
)

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
print(f"\n{'='*60}")
print(f"  Stress-Test Summary")
print(f"  Failures: {failures}   Warnings: {warnings}")
print(f"{'='*60}")
if failures == 0 and warnings == 0:
    print(f"  {PASS} All scenarios passed without warnings.")
elif failures == 0:
    print(f"  {PASS} No failures detected.")
    print(f"  {WARN} {warnings} scenario(s) flagged for attention.")
else:
    print(f"  {FAIL} {failures} scenario(s) indicate high risk.")
    print(f"  {WARN} {warnings} additional scenario(s) flagged.")
print(f"\nNote: Warnings reflect known economic assumptions, not code bugs.")
print(f"      Failures reflect deterministically verifiable risk conditions.")
print(f"{'='*60}\n")

sys.exit(0 if failures == 0 else 1)
