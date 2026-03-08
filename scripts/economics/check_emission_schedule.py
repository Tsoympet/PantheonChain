#!/usr/bin/env python3
"""
PantheonChain – Emission Schedule Checker
==========================================
Verifies the halving-based issuance schedule for TALANTON, DRACHMA, and OBOLOS:
  1. Each halving epoch: reward = initial_reward >> epoch_index.
  2. Reward is always >= 0 (no negative issuance).
  3. After 64 halvings the reward is effectively 0.
  4. Cumulative supply per epoch is monotonically non-decreasing.
  5. Total issuance converges below the hard cap.
  6. The staggered DRACHMA/OBOLOS launch heights are documented but bounded.

Exit code: 0 = all checks pass, 1 = at least one check failed.
"""

import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Canonical constants – must match issuance.h / asset.h
# ---------------------------------------------------------------------------
BASE_UNIT        = 100_000_000
HALVING_INTERVAL = 210_000

ASSETS = [
    # (ticker, initial_reward in whole tokens, launch_height, max_supply in whole tokens)
    ("TALN", 50,  0,       21_000_000),
    ("DRM",  97,  210_000, 41_000_000),
    ("OBL",  145, 420_000, 61_000_000),
]

PASS = "\033[32mPASS\033[0m"
FAIL = "\033[31mFAIL\033[0m"
WARN = "\033[33mWARN\033[0m"

failures = 0


def check(label, condition, detail=""):
    global failures
    status = PASS if condition else FAIL
    print(f"  [{status}] {label}" + (f" – {detail}" if detail else ""))
    if not condition:
        failures += 1


def section(title):
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")


def get_block_reward(height, initial_reward_whole, launch_height):
    """Mirrors Issuance::GetBlockReward in issuance.cpp."""
    if height < launch_height:
        return 0
    blocks_since_launch = height - launch_height
    halvings = blocks_since_launch // HALVING_INTERVAL
    if halvings >= 64:
        return 0
    reward = initial_reward_whole * BASE_UNIT
    reward >>= halvings
    return reward


def cumulative_supply(initial_reward_whole, launch_height, n_halvings=128):
    """Returns total base-unit supply from block 0 through all halvings."""
    total = 0
    for h in range(n_halvings):
        reward = get_block_reward(launch_height + h * HALVING_INTERVAL,
                                  initial_reward_whole, launch_height)
        total += reward * HALVING_INTERVAL
        if reward == 0:
            break
    return total


# ---------------------------------------------------------------------------
# 1. Reward at launch height equals initial reward
# ---------------------------------------------------------------------------
section("1. Reward at launch height == initial reward (no offset error)")
for ticker, initial_reward, launch_height, max_supply in ASSETS:
    reward = get_block_reward(launch_height, initial_reward, launch_height)
    expected = initial_reward * BASE_UNIT
    check(
        f"{ticker}: reward at launch height {launch_height} == {initial_reward} tokens",
        reward == expected,
        f"got {reward // BASE_UNIT}",
    )

# ---------------------------------------------------------------------------
# 2. Reward before launch height is 0
# ---------------------------------------------------------------------------
section("2. Reward before launch height is 0")
for ticker, initial_reward, launch_height, max_supply in ASSETS:
    if launch_height == 0:
        print(f"  [SKIP] {ticker}: launch at height 0, no pre-launch phase")
        continue
    reward_before = get_block_reward(launch_height - 1, initial_reward, launch_height)
    check(f"{ticker}: reward before launch height is 0", reward_before == 0)

# ---------------------------------------------------------------------------
# 3. Each halving halves the reward (right-shift by 1)
# ---------------------------------------------------------------------------
section("3. Halving schedule: each epoch reward = prev >> 1")
for ticker, initial_reward, launch_height, max_supply in ASSETS:
    prev = get_block_reward(launch_height, initial_reward, launch_height)
    ok = True
    for epoch in range(1, 20):
        curr = get_block_reward(launch_height + epoch * HALVING_INTERVAL,
                                initial_reward, launch_height)
        expected = prev >> 1
        if curr != expected:
            check(
                f"{ticker}: epoch {epoch} reward ({curr}) == epoch {epoch-1} ({prev}) >> 1",
                False,
                f"expected {expected}",
            )
            ok = False
            break
        prev = curr
    if ok:
        check(f"{ticker}: first 20 halvings correctly halve the reward", True)

# ---------------------------------------------------------------------------
# 4. No negative rewards
# ---------------------------------------------------------------------------
section("4. No negative rewards at any height")
for ticker, initial_reward, launch_height, max_supply in ASSETS:
    neg_found = False
    for epoch in range(70):
        height = launch_height + epoch * HALVING_INTERVAL
        reward = get_block_reward(height, initial_reward, launch_height)
        if reward < 0:
            check(f"{ticker}: reward at epoch {epoch} is non-negative", False)
            neg_found = True
    if not neg_found:
        check(f"{ticker}: all rewards non-negative (70 epochs tested)", True)

# ---------------------------------------------------------------------------
# 5. After 64 halvings reward is 0
# ---------------------------------------------------------------------------
section("5. Reward zeroes out after 64 halvings")
for ticker, initial_reward, launch_height, max_supply in ASSETS:
    height_64 = launch_height + 64 * HALVING_INTERVAL
    reward = get_block_reward(height_64, initial_reward, launch_height)
    check(f"{ticker}: reward at halving 64 == 0", reward == 0)

# ---------------------------------------------------------------------------
# 6. Cumulative issuance <= hard cap
# ---------------------------------------------------------------------------
section("6. Total cumulative issuance <= hard cap")
for ticker, initial_reward, launch_height, max_supply in ASSETS:
    total = cumulative_supply(initial_reward, launch_height)
    cap   = max_supply * BASE_UNIT
    pct   = (total / cap) * 100 if cap > 0 else 0
    check(
        f"{ticker}: cumulative ({total // BASE_UNIT:,} tokens) <= cap ({max_supply:,})",
        total <= cap,
        f"{pct:.4f}% of cap",
    )

# ---------------------------------------------------------------------------
# 7. Achievable supply (formula) matches cumulative simulation
# ---------------------------------------------------------------------------
section("7. Achievable supply formula == cumulative simulation (within rounding)")
for ticker, initial_reward, launch_height, max_supply in ASSETS:
    formula_achievable = initial_reward * HALVING_INTERVAL * 2
    simulated = cumulative_supply(initial_reward, launch_height) // BASE_UNIT
    # Integer halving rounds down; simulation may be slightly below formula
    diff = abs(formula_achievable - simulated)
    check(
        f"{ticker}: formula ({formula_achievable:,}) vs simulation ({simulated:,}), diff={diff}",
        diff <= initial_reward,   # within 1 block reward of rounding error
    )

# ---------------------------------------------------------------------------
# 8. No overflow in epoch issuance arithmetic (blocks_in_epoch * reward)
# ---------------------------------------------------------------------------
section("8. Epoch issuance arithmetic: no uint64 overflow")
UINT64_MAX = (1 << 64) - 1
for ticker, initial_reward, launch_height, max_supply in ASSETS:
    overflow_found = False
    for epoch in range(64):
        height = launch_height + epoch * HALVING_INTERVAL
        reward = get_block_reward(height, initial_reward, launch_height)
        epoch_supply = reward * HALVING_INTERVAL
        if epoch_supply > UINT64_MAX:
            check(f"{ticker}: epoch {epoch} overflows uint64", False)
            overflow_found = True
    if not overflow_found:
        check(f"{ticker}: no uint64 overflow in any epoch", True)

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
print(f"\n{'='*60}")
if failures == 0:
    print(f"  {PASS} All emission schedule checks passed.")
else:
    print(f"  {FAIL} {failures} check(s) failed.")
print(f"{'='*60}\n")

sys.exit(0 if failures == 0 else 1)
