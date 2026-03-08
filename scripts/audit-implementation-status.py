#!/usr/bin/env python3
"""PantheonChain repository reality-check audit.

This script validates high-signal implementation invariants for the intended
layered architecture:
  OBOLOS (L3) -> DRACHMA (L2) -> TALANTON (L1)

It intentionally checks only concrete artifacts in-repo and fails when
consensus/layer/token/checkpoint metadata diverges.
"""

from __future__ import annotations

import json
import pathlib
import re
import sys
from typing import Any

ROOT = pathlib.Path(__file__).resolve().parents[1]

PASS = "\033[32mPASS\033[0m"
FAIL = "\033[31mFAIL\033[0m"

errors = 0


def ok(label: str) -> None:
    print(f"[{PASS}] {label}")


def bad(label: str) -> None:
    global errors
    errors += 1
    print(f"[{FAIL}] {label}")


def load_json(path: pathlib.Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:  # noqa: BLE001
        bad(f"{path}: invalid JSON ({exc})")
        return {}


def check(condition: bool, label: str) -> None:
    if condition:
        ok(label)
    else:
        bad(label)


print("== PantheonChain implementation reality-check ==")

# 1) Genesis layer identity and consensus role.
expected_genesis = {
    "genesis_talanton.json": ("l1", "TALANTON", "pow-sha256d"),
    "genesis_drachma.json": ("l2", "DRACHMA", "pos-bft"),
    "genesis_obolos.json": ("l3", "OBOLOS", "pos-bft"),
}
for file_name, (layer, chain, consensus) in expected_genesis.items():
    data = load_json(ROOT / file_name)
    check(data.get("layer") == layer, f"{file_name}: layer={layer}")
    check(data.get("chain") == chain, f"{file_name}: chain={chain}")
    check(data.get("consensus") == consensus, f"{file_name}: consensus={consensus}")

# 2) Minimum-stake hardening sanity for PoS layers.
for file_name, min_expected in (("genesis_drachma.json", 100000), ("genesis_obolos.json", 50000)):
    data = load_json(ROOT / file_name)
    check(int(data.get("minimum_stake", 0)) >= min_expected,
          f"{file_name}: minimum_stake >= {min_expected} (Sybil hardening)")

# 3) Canonical checkpoint route in layer model.
layer_model = load_json(ROOT / "configs/layer-model.json")
check(layer_model.get("checkpoint_path") == ["l3", "l2", "l1"],
      "configs/layer-model.json: checkpoint_path is l3->l2->l1")

# 4) Network configs preserve cadence l3 < l2 < l1 and proper genesis binding.
for network in ("devnet", "testnet", "mainnet"):
    base = ROOT / "configs" / network
    l1 = load_json(base / "l1.json")
    l2 = load_json(base / "l2.json")
    l3 = load_json(base / "l3.json")

    check(l1.get("genesis_file") == "genesis_talanton.json",
          f"{network}: l1 genesis binds TALANTON")
    check(l2.get("genesis_file") == "genesis_drachma.json",
          f"{network}: l2 genesis binds DRACHMA")
    check(l3.get("genesis_file") == "genesis_obolos.json",
          f"{network}: l3 genesis binds OBOLOS")

    for cfg in (l1, l2, l3):
        check(int(cfg.get("checkpoint_freshness_slo_seconds", 0)) == 300,
              f"{network}: checkpoint_freshness_slo_seconds=300")
        check(int(cfg.get("relayer_liveness_threshold_seconds", 0)) == 120,
              f"{network}: relayer_liveness_threshold_seconds=120")

    cadence = (int(l3.get("commitment_interval", 0)),
               int(l2.get("commitment_interval", 0)),
               int(l1.get("commitment_interval", 0)))
    check(cadence[0] < cadence[1] < cadence[2],
          f"{network}: checkpoint cadence l3 < l2 < l1 ({cadence[0]}<{cadence[1]}<{cadence[2]})")

# 5) Relayer binaries enforce route direction by source chain checks.
l2_validator = (ROOT / "layer1-talanton/tx/l1_commitment_validator.cpp").read_text(encoding="utf-8")
check("SourceChain::DRACHMA" in l2_validator,
      "TALANTON L2 validator requires DRACHMA commitments")

l3_validator = (ROOT / "layer2-drachma/consensus/pos_consensus.cpp").read_text(encoding="utf-8")
check("SourceChain::OBOLOS" in l3_validator,
      "DRACHMA L3 validator requires OBOLOS commitments")

# 6) docs must explicitly state that TALANTON is root settlement.
for doc in ("README.md", "docs/architecture.md", "docs/SETTLEMENT_AND_FINALITY.md"):
    text = (ROOT / doc).read_text(encoding="utf-8")
    matches = bool(re.search(r"TALANTON[^\n]*(root settlement|ultimate settlement|root trust)|(root settlement|ultimate settlement|root trust)[^\n]*TALANTON", text, re.IGNORECASE))
    check(matches, f"{doc}: explicitly declares TALANTON as root settlement")

# 7) No docs should claim trustless bridging in MVP security docs.
for doc in ("README.md", "docs/SECURITY_MODEL.md", "docs/threat-model.md"):
    text = (ROOT / doc).read_text(encoding="utf-8")
    violating = False
    for m in re.finditer(r"\btrustless\s+bridg", text, re.IGNORECASE):
        window = text[max(0, m.start()-60): m.end()+60].lower()
        if not any(phrase in window for phrase in ("no trustless", "not trustless", "no claim of trustless", "without trustless")):
            violating = True
            break
    if violating:
        bad(f"{doc}: contains unqualified trustless-bridging claim")
    else:
        ok(f"{doc}: no unqualified trustless-bridging claim")

if errors:
    print(f"\nAudit failed with {errors} issue(s).")
    sys.exit(1)

print("\nAll architecture reality-check assertions passed.")
