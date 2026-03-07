#!/usr/bin/env python3
"""
PantheonChain – Supply Cap Invariant Checker
============================================
Verifies that:
  1. All genesis/config supply figures are consistent with hard caps.
  2. The TALANTON, DRACHMA, and OBOLOS supply constants in the C++ headers
     are internally coherent (caps, achievable supply, initial rewards).
  3. Cumulative halving-schedule issuance never exceeds the hard cap.

Exit code: 0 = all checks pass, 1 = at least one check failed.
"""

import json
import math
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Canonical constants – must match layer1-talanton/core/primitives/asset.h
# and layer1-talanton/core/consensus/issuance.h
# ---------------------------------------------------------------------------
BASE_UNIT = 100_000_000          # 1 token = 1e8 base units (8 decimals)
HALVING_INTERVAL = 210_000       # blocks per halving epoch

TALN_INITIAL_REWARD = 50         # whole tokens per block
DRM_INITIAL_REWARD  = 97
OBL_INITIAL_REWARD  = 145

TALN_MAX_SUPPLY = 21_000_000     # whole tokens
DRM_MAX_SUPPLY  = 41_000_000
OBL_MAX_SUPPLY  = 61_000_000

TALN_ACHIEVABLE_SUPPLY = TALN_INITIAL_REWARD * HALVING_INTERVAL * 2   # = 21,000,000
DRM_ACHIEVABLE_SUPPLY  = DRM_INITIAL_REWARD  * HALVING_INTERVAL * 2   # = 40,740,000
OBL_ACHIEVABLE_SUPPLY  = OBL_INITIAL_REWARD  * HALVING_INTERVAL * 2   # = 60,900,000

ASSETS = [
    {"name": "TALN", "initial_reward": TALN_INITIAL_REWARD,
     "max_supply": TALN_MAX_SUPPLY, "achievable": TALN_ACHIEVABLE_SUPPLY},
    {"name": "DRM",  "initial_reward": DRM_INITIAL_REWARD,
     "max_supply": DRM_MAX_SUPPLY,  "achievable": DRM_ACHIEVABLE_SUPPLY},
    {"name": "OBL",  "initial_reward": OBL_INITIAL_REWARD,
     "max_supply": OBL_MAX_SUPPLY,  "achievable": OBL_ACHIEVABLE_SUPPLY},
]

REPO_ROOT = Path(__file__).resolve().parents[2]

PASS = "\033[32mPASS\033[0m"
FAIL = "\033[31mFAIL\033[0m"
WARN = "\033[33mWARN\033[0m"

failures = 0


def check(label, condition, detail=""):
    global failures
    status = PASS if condition else FAIL
    print(f"  [{status}] {label}" + (f": {detail}" if detail else ""))
    if not condition:
        failures += 1


def section(title):
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")


# ---------------------------------------------------------------------------
# 1. Achievable vs cap coherence
# ---------------------------------------------------------------------------
section("1. Achievable supply <= hard cap for each token")
for a in ASSETS:
    check(
        f"{a['name']}: achievable ({a['achievable']:,}) <= cap ({a['max_supply']:,})",
        a["achievable"] <= a["max_supply"],
    )
    gap = a["max_supply"] - a["achievable"]
    pct = (a["achievable"] / a["max_supply"]) * 100
    print(f"       gap = {gap:,} tokens  ({pct:.4f}% of cap achievable)")

# ---------------------------------------------------------------------------
# 2. Cumulative halving-schedule issuance never exceeds cap
# ---------------------------------------------------------------------------
section("2. Cumulative issuance <= hard cap (all halvings)")
for a in ASSETS:
    cumulative = 0
    exceeded = False
    for halving in range(128):
        reward = a["initial_reward"] >> halving   # integer right-shift
        cumulative += reward * HALVING_INTERVAL
        if cumulative > a["max_supply"]:
            exceeded = True
            check(
                f"{a['name']}: cap exceeded at halving {halving} "
                f"(cumulative={cumulative:,}, cap={a['max_supply']:,})",
                False,
            )
            break
        if reward == 0:
            break
    if not exceeded:
        check(
            f"{a['name']}: cumulative ({cumulative:,}) <= cap ({a['max_supply']:,})",
            True,
        )

# ---------------------------------------------------------------------------
# 3. Genesis JSON files: no explicit supply field exceeds the hard cap
# ---------------------------------------------------------------------------
section("3. Genesis JSON files – supply fields vs hard caps")
genesis_files = {
    "genesis_talanton.json": ("TALN", TALN_MAX_SUPPLY),
    "genesis_drachma.json":  ("DRM",  DRM_MAX_SUPPLY),
    "genesis_obolos.json":   ("OBL",  OBL_MAX_SUPPLY),
}
for fname, (ticker, cap) in genesis_files.items():
    gpath = REPO_ROOT / fname
    if not gpath.exists():
        print(f"  [SKIP] {fname}: file not found")
        continue
    with open(gpath) as f:
        genesis = json.load(f)

    # Check for any explicit supply or premine fields
    for key in ("initial_supply", "premine", "genesis_supply", "max_supply"):
        if key in genesis:
            val = genesis[key]
            check(
                f"{fname}[{key}] ({val:,}) <= {ticker} cap ({cap:,})",
                val <= cap,
            )
    print(f"  [INFO] {fname}: no explicit supply fields found (params via config)")

# ---------------------------------------------------------------------------
# 4. Supply-policy header consistency (parsed from C++ source)
# ---------------------------------------------------------------------------
section("4. Supply-policy constants (supply_policy.h) vs canonical values")
sp_path = REPO_ROOT / "layer1-talanton" / "governance" / "supply_policy.h"
if sp_path.exists():
    src = sp_path.read_text()

    def extract_constexpr(source, name):
        """Extract an integer constant from a C++ constexpr declaration."""
        import re
        # Match: static constexpr uint64_t NAME = <digits with optional separators>ULL;
        pattern = rf"constexpr\s+uint64_t\s+{re.escape(name)}\s*=\s*([\d']+)ULL"
        m = re.search(pattern, source)
        if not m:
            return None
        return int(m.group(1).replace("'", ""))

    sp_taln_max = extract_constexpr(src, "TALN_MAX_SUPPLY")
    sp_drm_max  = extract_constexpr(src, "DRM_MAX_SUPPLY")
    sp_obl_max  = extract_constexpr(src, "OBL_MAX_SUPPLY")
    sp_drm_ach  = extract_constexpr(src, "DRM_ACHIEVABLE_SUPPLY")
    sp_obl_ach  = extract_constexpr(src, "OBL_ACHIEVABLE_SUPPLY")

    # The regex extracts the whole-token multiplier (the part before * BASE_UNIT).
    # Compare directly to canonical whole-token constants.
    if sp_taln_max is not None:
        check(
            f"supply_policy.h TALN_MAX_SUPPLY multiplier ({sp_taln_max:,}) == {TALN_MAX_SUPPLY:,}",
            sp_taln_max == TALN_MAX_SUPPLY,
        )
    if sp_drm_max is not None:
        check(
            f"supply_policy.h DRM_MAX_SUPPLY multiplier ({sp_drm_max:,}) == {DRM_MAX_SUPPLY:,}",
            sp_drm_max == DRM_MAX_SUPPLY,
        )
    if sp_obl_max is not None:
        check(
            f"supply_policy.h OBL_MAX_SUPPLY multiplier ({sp_obl_max:,}) == {OBL_MAX_SUPPLY:,}",
            sp_obl_max == OBL_MAX_SUPPLY,
        )
    if sp_drm_ach is not None:
        check(
            f"supply_policy.h DRM_ACHIEVABLE_SUPPLY multiplier ({sp_drm_ach:,}) == {DRM_ACHIEVABLE_SUPPLY:,}",
            sp_drm_ach == DRM_ACHIEVABLE_SUPPLY,
        )
    if sp_obl_ach is not None:
        check(
            f"supply_policy.h OBL_ACHIEVABLE_SUPPLY multiplier ({sp_obl_ach:,}) == {OBL_ACHIEVABLE_SUPPLY:,}",
            sp_obl_ach == OBL_ACHIEVABLE_SUPPLY,
        )
else:
    print(f"  [SKIP] supply_policy.h not found at expected path")

# ---------------------------------------------------------------------------
# 5. asset.h constants vs canonical values
# ---------------------------------------------------------------------------
section("5. asset.h constants vs canonical values")
asset_path = REPO_ROOT / "layer1-talanton" / "core" / "primitives" / "asset.h"
if asset_path.exists():
    src = asset_path.read_text()

    def extract_ull(source, name):
        import re
        pattern = rf"constexpr\s+uint64_t\s+{re.escape(name)}\s*=\s*([\d']+)ULL\s*\*\s*BASE_UNIT"
        m = re.search(pattern, source)
        if not m:
            return None
        return int(m.group(1).replace("'", ""))

    ah_taln_max = extract_ull(src, "TALN_MAX_SUPPLY")
    ah_drm_max  = extract_ull(src, "DRM_MAX_SUPPLY")
    ah_obl_max  = extract_ull(src, "OBL_MAX_SUPPLY")
    ah_drm_ach  = extract_ull(src, "DRM_ACHIEVABLE_SUPPLY")
    ah_obl_ach  = extract_ull(src, "OBL_ACHIEVABLE_SUPPLY")

    # The regex extracts the whole-token multiplier (part before * BASE_UNIT).
    if ah_taln_max is not None:
        check(f"asset.h TALN_MAX_SUPPLY multiplier ({ah_taln_max:,}) == {TALN_MAX_SUPPLY:,}",
              ah_taln_max == TALN_MAX_SUPPLY)
    if ah_drm_max is not None:
        check(f"asset.h DRM_MAX_SUPPLY multiplier ({ah_drm_max:,}) == {DRM_MAX_SUPPLY:,}",
              ah_drm_max == DRM_MAX_SUPPLY)
    if ah_obl_max is not None:
        check(f"asset.h OBL_MAX_SUPPLY multiplier ({ah_obl_max:,}) == {OBL_MAX_SUPPLY:,}",
              ah_obl_max == OBL_MAX_SUPPLY)
    if ah_drm_ach is not None:
        check(f"asset.h DRM_ACHIEVABLE_SUPPLY multiplier ({ah_drm_ach:,}) == {DRM_ACHIEVABLE_SUPPLY:,}",
              ah_drm_ach == DRM_ACHIEVABLE_SUPPLY)
    if ah_obl_ach is not None:
        check(f"asset.h OBL_ACHIEVABLE_SUPPLY multiplier ({ah_obl_ach:,}) == {OBL_ACHIEVABLE_SUPPLY:,}",
              ah_obl_ach == OBL_ACHIEVABLE_SUPPLY)
else:
    print(f"  [SKIP] asset.h not found at expected path")

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
print(f"\n{'='*60}")
if failures == 0:
    print(f"  {PASS} All supply cap checks passed.")
else:
    print(f"  {FAIL} {failures} check(s) failed.")
print(f"{'='*60}\n")

sys.exit(0 if failures == 0 else 1)
