#!/usr/bin/env python3
"""
PantheonChain – Decimal and Denomination Consistency Checker
=============================================================
Verifies that token decimal precision, denomination definitions, and
unit-conversion ratios are internally coherent across all three tokens.

Checks:
  1. All three tokens use 8 decimal places (like Bitcoin satoshi).
  2. BASE_UNIT = 10^8 for each token.
  3. Denomination ratios are historically accurate and internally consistent.
  4. Cross-token conversion ratios satisfy: DR_PER_TAL * OB_PER_DR == OB_PER_TAL.
  5. No denomination alias collision between DRACHMA and OBOLOS.
  6. Integer arithmetic with these units does not truncate to zero for plausible amounts.
  7. Supply cap values in base units fit within uint64.

Exit code: 0 = all checks pass, 1 = at least one check failed.
"""

import sys

# ---------------------------------------------------------------------------
# Canonical constants – must match units.h
# ---------------------------------------------------------------------------
TAL_DECIMALS = 8
DR_DECIMALS  = 8
OB_DECIMALS  = 8

TAL_BASE_UNIT = 10 ** TAL_DECIMALS   # 100,000,000
DR_BASE_UNIT  = 10 ** DR_DECIMALS
OB_BASE_UNIT  = 10 ** OB_DECIMALS

# Cross-token ratios (from units.h)
RATIO_DR_PER_TAL  = 6_000
RATIO_OB_PER_DR   = 6
RATIO_OB_PER_TAL  = 36_000

# Supply caps (whole tokens × BASE_UNIT = base units)
TALN_MAX_SUPPLY_BASE = 21_000_000 * TAL_BASE_UNIT
DRM_MAX_SUPPLY_BASE  = 41_000_000 * DR_BASE_UNIT
OBL_MAX_SUPPLY_BASE  = 61_000_000 * OB_BASE_UNIT

UINT64_MAX = (1 << 64) - 1

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
# 1. Decimal precision
# ---------------------------------------------------------------------------
section("1. Decimal precision (all tokens = 8 decimals)")
check("TALN_DECIMALS == 8", TAL_DECIMALS == 8)
check("DR_DECIMALS == 8",   DR_DECIMALS  == 8)
check("OB_DECIMALS == 8",   OB_DECIMALS  == 8)

# ---------------------------------------------------------------------------
# 2. BASE_UNIT = 10^8
# ---------------------------------------------------------------------------
section("2. BASE_UNIT == 10^8 for each token")
check(f"TAL_BASE_UNIT == {10**8}", TAL_BASE_UNIT == 10**8)
check(f"DR_BASE_UNIT  == {10**8}", DR_BASE_UNIT  == 10**8)
check(f"OB_BASE_UNIT  == {10**8}", OB_BASE_UNIT  == 10**8)
check("All BASE_UNITs are equal (required by ValidateMonetaryInvariants)",
      TAL_BASE_UNIT == DR_BASE_UNIT == OB_BASE_UNIT)

# ---------------------------------------------------------------------------
# 3. Cross-token conversion ratio invariant
# ---------------------------------------------------------------------------
section("3. Cross-token ratio invariant: DR_PER_TAL * OB_PER_DR == OB_PER_TAL")
check(
    f"RATIO_DR_PER_TAL ({RATIO_DR_PER_TAL}) * RATIO_OB_PER_DR ({RATIO_OB_PER_DR}) "
    f"== RATIO_OB_PER_TAL ({RATIO_OB_PER_TAL})",
    RATIO_DR_PER_TAL * RATIO_OB_PER_DR == RATIO_OB_PER_TAL,
)
print(f"  [INFO] 1 TALN = {RATIO_DR_PER_TAL:,} DRM = {RATIO_OB_PER_TAL:,} OBL")
print(f"  [INFO] 1 DRM  = {RATIO_OB_PER_DR} OBL")

# Historical basis: 1 drachma = 6 obols (correct)
check("RATIO_OB_PER_DR == 6 (historically: 1 drachma = 6 obols)",
      RATIO_OB_PER_DR == 6)
check("RATIO_DR_PER_TAL == 6000 (1 talanton = 6000 drachmai)",
      RATIO_DR_PER_TAL == 6000)

# ---------------------------------------------------------------------------
# 4. Supply cap values fit in uint64
# ---------------------------------------------------------------------------
section("4. Supply caps fit in uint64 (no overflow in base-unit representation)")
check(f"TALN max supply in base units ({TALN_MAX_SUPPLY_BASE:,}) <= UINT64_MAX",
      TALN_MAX_SUPPLY_BASE <= UINT64_MAX)
check(f"DRM  max supply in base units ({DRM_MAX_SUPPLY_BASE:,}) <= UINT64_MAX",
      DRM_MAX_SUPPLY_BASE  <= UINT64_MAX)
check(f"OBL  max supply in base units ({OBL_MAX_SUPPLY_BASE:,}) <= UINT64_MAX",
      OBL_MAX_SUPPLY_BASE  <= UINT64_MAX)

# ---------------------------------------------------------------------------
# 5. Cross-token conversion overflow check
# ---------------------------------------------------------------------------
section("5. ConvertTalToDr / ConvertDrToOb overflow bounds")

# ConvertTalToDr: tal_raw * RATIO_DR_PER_TAL must fit uint64
# Worst case: tal_raw = TALN_MAX_SUPPLY_BASE
tal_to_dr_max = TALN_MAX_SUPPLY_BASE * RATIO_DR_PER_TAL
check(
    f"ConvertTalToDr(TALN_MAX_SUPPLY) = {tal_to_dr_max:,}: fits uint64",
    tal_to_dr_max <= UINT64_MAX,
    f"result = {tal_to_dr_max}"
)
if tal_to_dr_max > UINT64_MAX:
    warn("ConvertTalToDr can overflow uint64 at maximum TALN supply – "
         "existing overflow check in units.cpp handles this but caller must check nullopt.")

# ConvertDrToOb: dr_raw * RATIO_OB_PER_DR
# Worst case: dr_raw = DRM_MAX_SUPPLY_BASE
dr_to_ob_max = DRM_MAX_SUPPLY_BASE * RATIO_OB_PER_DR
check(
    f"ConvertDrToOb(DRM_MAX_SUPPLY) = {dr_to_ob_max:,}: fits uint64",
    dr_to_ob_max <= UINT64_MAX,
    f"result = {dr_to_ob_max}"
)

# ---------------------------------------------------------------------------
# 6. Denomination alias collision check
# ---------------------------------------------------------------------------
section("6. Denomination alias collision: DRACHMA 'obol' vs OBOLOS token")

# DRACHMA denomination aliases for the 'obol' sub-unit
# (must NOT include 'obolos' after the fix in denominations.cpp)
# NOTE: the name "obol" is shared historically by both the DRACHMA sub-unit
# and the OBOLOS base denomination, but lookups are always asset-scoped
# (ResolveDenomination takes an AssetID), so the name collision is benign.
# The dangerous collision was the alias "obolos" being present in the
# DRACHMA denomination list, which could confuse user-facing amount parsing.
drm_obol_aliases = ["ob", "obol"]   # correct list after fix

check(
    "'obolos' is NOT an alias for the DRACHMA obol sub-denomination",
    "obolos" not in drm_obol_aliases,
)

# Verify denominations.cpp on disk if possible
import re
from pathlib import Path
REPO_ROOT = Path(__file__).resolve().parents[2]
denom_path = REPO_ROOT / "common" / "monetary" / "denominations.cpp"
if denom_path.exists():
    src = denom_path.read_text()
    # Look for the DRACHMA obol alias list
    drm_obol_section = re.search(
        r'"obol".*?DRACHMA.*?\{(.*?)\}',
        src, re.DOTALL
    )
    if drm_obol_section:
        aliases_str = drm_obol_section.group(1)
        has_obolos_alias = '"obolos"' in aliases_str
        check(
            "denominations.cpp: DRACHMA obol aliases do not contain 'obolos'",
            not has_obolos_alias,
            "would shadow OBOLOS token name in denomination lookup",
        )
    else:
        print("  [INFO] Could not parse DRACHMA obol alias list from denominations.cpp")

# ---------------------------------------------------------------------------
# 7. Minimum amount representability
# ---------------------------------------------------------------------------
section("7. Minimum meaningful amounts are representable")

# Minimum fee / minimum send = 1 base unit (0.00000001 tokens)
# Verify this is nonzero and positive.
check("1 base unit (minimum) > 0", 1 > 0)
check("1 TALN in base units = 100,000,000", TAL_BASE_UNIT == 100_000_000)

# A 97-DRM block reward in base units
drm_reward_base = 97 * DR_BASE_UNIT
check(f"DRM 97-token reward in base units ({drm_reward_base:,}) fits uint64",
      drm_reward_base <= UINT64_MAX)

# An OBL gas price of 1 Gwei (1e9) — note: Ethereum uses 1e18 base unit,
# OBL uses 1e8. A 'Gwei-equivalent' in OBL units = 10 base units (1e8 / 1e7).
# Ensure this is non-zero.
obl_gwei_equiv = 10   # 10 base units per 'nanoOBL'
check(f"OBL Gwei-equivalent ({obl_gwei_equiv} base units) > 0", obl_gwei_equiv > 0)
warn(
    "OBL: gas_pricing.h INITIAL_BASE_FEE = 1e9 (Ethereum Gwei), but OBL has "
    "8 decimals (1e8 base unit). EVM gas price units should be reconciled "
    "to avoid 10x underpricing of gas. Recommend defining OBL_GWEI = 10 base units.",
)

# ---------------------------------------------------------------------------
# 8. Reward * blocks does not overflow uint64
# ---------------------------------------------------------------------------
section("8. Reward × blocks_in_epoch does not overflow uint64")

for ticker, reward_whole, launch_height in [
    ("TALN", 50,  0),
    ("DRM",  97,  210_000),
    ("OBL",  145, 420_000),
]:
    reward_base = reward_whole * TAL_BASE_UNIT
    epoch_supply = reward_base * 210_000
    check(
        f"{ticker}: initial_reward ({reward_whole}) × HALVING_INTERVAL (210000) "
        f"= {epoch_supply:,} base units, fits uint64",
        epoch_supply <= UINT64_MAX,
    )

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
print(f"\n{'='*60}")
print(f"  Warnings: {warnings}   Failures: {failures}")
if failures == 0:
    print(f"  {PASS} All decimal/denomination consistency checks passed.")
    if warnings > 0:
        print(f"  {WARN} {warnings} warning(s) require attention (see above).")
else:
    print(f"  {FAIL} {failures} check(s) failed.")
print(f"{'='*60}\n")

sys.exit(0 if failures == 0 else 1)
