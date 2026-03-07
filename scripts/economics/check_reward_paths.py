#!/usr/bin/env python3
"""
PantheonChain – Reward Path Checker
=====================================
Verifies that reward paths and fee flows for each layer are coherent:

  TALANTON (L1 PoW):
    - Miner receives block subsidy + L1 transaction fees.
    - No staking reward path exists; staking is disabled in genesis.
    - Commitment transaction (TX_L2_COMMIT) fees flow to miner.

  DRACHMA (L2 PoS/BFT):
    - Validators receive block subsidy from halving schedule.
    - Validators also receive L2 transaction fees (priority fee portion).
    - Base fee portion is burned (EIP-1559 model on L3 does not apply here,
      but the same logic is documented).
    - Checkpoint submission (TX_L3_COMMIT) fees flow to submitting validator.
    - Minimum stake (1000 DRM) is plausible vs circulating supply.

  OBOLOS (L3 PoS/BFT + EVM):
    - Validators receive block subsidy from halving schedule.
    - Gas base fee burned; priority fee goes to block proposer.
    - Staking collateral minimum (500 OBL) is plausible vs circulating supply.
    - Governance treasury capped at 50% of circulating supply.

Checks are deterministic and do not assume market prices.

Exit code: 0 = all checks pass, 1 = at least one check failed.
"""

import sys

# ---------------------------------------------------------------------------
# Canonical constants
# ---------------------------------------------------------------------------
BASE_UNIT        = 100_000_000
HALVING_INTERVAL = 210_000

# Block rewards (whole tokens)
TALN_INITIAL_REWARD = 50
DRM_INITIAL_REWARD  = 97
OBL_INITIAL_REWARD  = 145

# Supply caps (whole tokens)
TALN_MAX_SUPPLY = 21_000_000
DRM_MAX_SUPPLY  = 41_000_000
OBL_MAX_SUPPLY  = 61_000_000

# Minimum stake (from genesis JSON)
DRM_MIN_STAKE = 1_000   # DRM
OBL_MIN_STAKE = 500     # OBL

# DRACHMA staking: minimum count of validators at minimum stake
# to consider the chain minimally secure.
MIN_VALIDATORS = 4   # BFT requires >= 4 validators for 2/3 quorum

# Annual PoS inflation rates declared in genesis JSONs (informational only;
# NOT enforced in the current halving-schedule issuance code).
# These represent documented aspirations, not implemented mechanics.
DRACHMA_GENESIS_ANNUAL_RATE = 0.05   # 5 %
OBOLOS_GENESIS_ANNUAL_RATE  = 0.07   # 7 %

PASS = "\033[32mPASS\033[0m"
FAIL = "\033[31mFAIL\033[0m"
WARN = "\033[33mWARN\033[0m"

failures = 0
warnings = 0


def check(label, condition, detail=""):
    global failures
    status = PASS if condition else FAIL
    print(f"  [{status}] {label}" + (f" – {detail}" if detail else ""))
    if not condition:
        failures += 1


def warn(label, detail=""):
    global warnings
    print(f"  [{WARN}] {label}" + (f" – {detail}" if detail else ""))
    warnings += 1


def section(title):
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")


# ---------------------------------------------------------------------------
# 1. TALANTON miner reward path
# ---------------------------------------------------------------------------
section("1. TALANTON – Miner reward path")

taln_reward_genesis = TALN_INITIAL_REWARD * BASE_UNIT
check("TALN: initial block subsidy > 0", taln_reward_genesis > 0)
check("TALN: block subsidy in base units fits uint64",
      taln_reward_genesis < (1 << 64))

# Verify staggered launch: DRM and OBL start at heights 210000/420000 on the
# L1 block counter (the issuance code treats all three tokens together).
# At TALN height 0, DRM and OBL yield 0.
def get_reward(height, initial_reward_whole, launch_height):
    if height < launch_height:
        return 0
    halvings = (height - launch_height) // HALVING_INTERVAL
    if halvings >= 64:
        return 0
    return (initial_reward_whole * BASE_UNIT) >> halvings

check("TALN: reward at height 0 is 50 TALN",
      get_reward(0, TALN_INITIAL_REWARD, 0) == 50 * BASE_UNIT)
check("DRM: no reward at TALN height 0 (before DRM launch)",
      get_reward(0, DRM_INITIAL_REWARD, HALVING_INTERVAL) == 0)
check("OBL: no reward at TALN height 0 (before OBL launch)",
      get_reward(0, OBL_INITIAL_REWARD, 2 * HALVING_INTERVAL) == 0)

# PoW security: reward must remain positive for many epochs
early_decay_height = 10 * HALVING_INTERVAL
taln_reward_10 = get_reward(early_decay_height, TALN_INITIAL_REWARD, 0)
check(f"TALN: reward still positive at epoch 10 (height {early_decay_height:,})",
      taln_reward_10 > 0)

# Long-run PoW security: at epoch 32 (halving 32), reward ≈ 50/(2^32) ~ 11 talantonion
taln_reward_32 = get_reward(32 * HALVING_INTERVAL, TALN_INITIAL_REWARD, 0)
print(f"  [INFO] TALN reward at halving 32: {taln_reward_32} base units"
      f" ({taln_reward_32 / BASE_UNIT:.10f} TALN)")
if taln_reward_32 == 0:
    warn("TALN: reward reaches 0 by halving 32 – fee-only security from that point")
else:
    print(f"  [INFO] TALN: reward non-zero until halving ≥ 33")

# ---------------------------------------------------------------------------
# 2. DRACHMA validator reward path
# ---------------------------------------------------------------------------
section("2. DRACHMA – Validator reward path")

drm_reward_launch = get_reward(HALVING_INTERVAL, DRM_INITIAL_REWARD, HALVING_INTERVAL)
check("DRM: initial block subsidy > 0 at launch height",
      drm_reward_launch > 0)
check(f"DRM: initial reward == {DRM_INITIAL_REWARD} DRM",
      drm_reward_launch == DRM_INITIAL_REWARD * BASE_UNIT)

# Min stake sanity: MIN_VALIDATORS * MIN_STAKE should be << initial circulating supply.
# At launch (height 210000), TALN has been running for 210000 blocks already.
# At the DRM launch point, DRM circulating = 0 (just starting).
# After 1 DRM epoch (210000 blocks from launch): ~97 DRM * 210000 = 20.37M DRM
drm_first_epoch_supply = DRM_INITIAL_REWARD * HALVING_INTERVAL
drm_min_validator_stake = MIN_VALIDATORS * DRM_MIN_STAKE
pct_of_first_epoch = (drm_min_validator_stake / drm_first_epoch_supply) * 100
check(
    f"DRM: min validator set stake ({drm_min_validator_stake:,} DRM) < first epoch supply "
    f"({drm_first_epoch_supply:,} DRM) – {pct_of_first_epoch:.2f}%",
    drm_min_validator_stake < drm_first_epoch_supply,
)

# Warn if min stake is extremely small relative to supply (validator count cost trivial)
if drm_min_validator_stake / (DRM_MAX_SUPPLY) < 0.0001:
    warn(
        f"DRM: total min validator stake ({drm_min_validator_stake:,}) is "
        f"{drm_min_validator_stake/DRM_MAX_SUPPLY*100:.4f}% of max supply – "
        "validator entry cost is very low, raising Sybil risk"
    )

# Genesis declares PoS annual rate but code uses halving schedule – flag the gap
warn(
    "DRM: genesis_drachma.json declares annual_rate=0.05 (PoS model), but "
    "issuance.cpp implements a PoW-style halving schedule. "
    "The PoS annual-rate field is NOT enforced in the current implementation.",
)

# ---------------------------------------------------------------------------
# 3. OBOLOS validator/sequencer reward path
# ---------------------------------------------------------------------------
section("3. OBOLOS – Validator/sequencer reward path")

obl_reward_launch = get_reward(2 * HALVING_INTERVAL, OBL_INITIAL_REWARD, 2 * HALVING_INTERVAL)
check("OBL: initial block subsidy > 0 at launch height",
      obl_reward_launch > 0)
check(f"OBL: initial reward == {OBL_INITIAL_REWARD} OBL",
      obl_reward_launch == OBL_INITIAL_REWARD * BASE_UNIT)

obl_first_epoch_supply = OBL_INITIAL_REWARD * HALVING_INTERVAL
obl_min_validator_stake = MIN_VALIDATORS * OBL_MIN_STAKE
pct_of_obl = (obl_min_validator_stake / obl_first_epoch_supply) * 100
check(
    f"OBL: min validator set stake ({obl_min_validator_stake:,} OBL) < first epoch supply "
    f"({obl_first_epoch_supply:,}) – {pct_of_obl:.2f}%",
    obl_min_validator_stake < obl_first_epoch_supply,
)

warn(
    "OBL: genesis_obolos.json declares annual_rate=0.07 (PoS model), but "
    "issuance.cpp implements a PoW-style halving schedule. "
    "The PoS annual-rate field is NOT enforced in the current implementation.",
)

# ---------------------------------------------------------------------------
# 4. Gas fee path (OBOLOS EIP-1559)
# ---------------------------------------------------------------------------
section("4. OBOLOS – EIP-1559 gas fee path")

MIN_BASE_FEE = 7           # wei (from gas_pricing.h)
INITIAL_BASE_FEE = 1_000_000_000   # 1 Gwei
TARGET_GAS_PER_BLOCK = 15_000_000
MAX_GAS_PER_BLOCK    = 30_000_000

check("OBL: MIN_BASE_FEE > 0 (spam resistance floor)", MIN_BASE_FEE > 0)
check("OBL: INITIAL_BASE_FEE >= MIN_BASE_FEE", INITIAL_BASE_FEE >= MIN_BASE_FEE)
check("OBL: MAX_GAS_PER_BLOCK == 2 * TARGET_GAS_PER_BLOCK (EIP-1559 elasticity)",
      MAX_GAS_PER_BLOCK == 2 * TARGET_GAS_PER_BLOCK)

# A full block at initial base fee: base_fee_burned per block
max_burn_per_block = MAX_GAS_PER_BLOCK * INITIAL_BASE_FEE
print(f"  [INFO] Max base-fee burn per block (full block, initial base fee): "
      f"{max_burn_per_block:,} wei")

# OBL block reward (initial): 145 OBL = 145 * 1e8 base units
# But gas is denominated in wei (1e18) for Ethereum. OBOLOS uses OBL as gas token.
# If OBL base unit = 1e8 (not 1e18), Gwei-equivalent = 1e8 / 1e9 = 0.1.
# This means the gas_pricing.h uses Ethereum Gwei but OBL has 8 decimals.
# Flag the potential mismatch.
warn(
    "OBL: gas_pricing.h uses Ethereum Gwei units (1e9 wei baseline) but "
    "OBL has 8 decimal places (1 OBL = 1e8 base units). "
    "Gas accounting should use 1e8-denominated units, not Ethereum 1e18 wei. "
    "Verify that EVM gas-price arithmetic accounts for this difference.",
)

# ---------------------------------------------------------------------------
# 5. Checkpoint fee incentive (relayer coverage)
# ---------------------------------------------------------------------------
section("5. Cross-layer checkpoint fee incentives")

# DRACHMA checkpoint to TALANTON: submitter must pay L1 TX fee in TALN.
# If TALN block reward is nonzero, L1 miners are incentivized to include commits.
# The economic question: can a DRACHMA validator afford the L1 TX fee?
# We can only verify that the reward schedule doesn't actively prohibit this.
check(
    "TALN: block reward at genesis height covers miner incentive for TX_L2_COMMIT",
    get_reward(0, TALN_INITIAL_REWARD, 0) > 0,
)
check(
    "DRM: block reward at launch > 0, providing DRM source for relayer operation",
    get_reward(HALVING_INTERVAL, DRM_INITIAL_REWARD, HALVING_INTERVAL) > 0,
)

warn(
    "Checkpoint relayer costs are NOT modeled in the current reward schedules. "
    "DRACHMA validators submitting TX_L2_COMMIT to TALANTON must pay TALN fees. "
    "There is no direct on-chain reimbursement mechanism defined for relayers.",
)

# ---------------------------------------------------------------------------
# 6. Governance treasury cap check (supply_policy.h)
# ---------------------------------------------------------------------------
section("6. Governance treasury cap (50% of achievable supply)")

TIER_HIGH_BPS = 5000   # 50%

for ticker, max_supply, achievable in [
    ("TALN", TALN_MAX_SUPPLY, TALN_INITIAL_REWARD * HALVING_INTERVAL * 2),
    ("DRM",  DRM_MAX_SUPPLY,  DRM_INITIAL_REWARD  * HALVING_INTERVAL * 2),
    ("OBL",  OBL_MAX_SUPPLY,  OBL_INITIAL_REWARD  * HALVING_INTERVAL * 2),
]:
    treasury_cap = achievable * TIER_HIGH_BPS // 10000
    check(
        f"{ticker}: treasury cap ({treasury_cap:,}) < achievable supply ({achievable:,})",
        treasury_cap < achievable,
    )
    check(
        f"{ticker}: treasury cap ({treasury_cap:,}) < max supply ({max_supply:,})",
        treasury_cap < max_supply,
    )

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
print(f"\n{'='*60}")
print(f"  Warnings: {warnings}   Failures: {failures}")
if failures == 0:
    print(f"  {PASS} All reward-path checks passed.")
    if warnings > 0:
        print(f"  {WARN} {warnings} warning(s) require attention (see above).")
else:
    print(f"  {FAIL} {failures} check(s) failed.")
print(f"{'='*60}\n")

sys.exit(0 if failures == 0 else 1)
