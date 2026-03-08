#!/usr/bin/env python3
"""Runtime checkpoint watchdog for PantheonChain layered devnet.

Monitors:
- checkpoint freshness per layer state file
- route consistency (L3 commitments observed at L2, L2 commitments observed at L1)

Intended for runtime enforcement (daemon mode) beyond static config validation.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import sys
import time
from typing import Any


ALERT = "ALERT"
OK = "OK"


def load_json(path: pathlib.Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:  # noqa: BLE001
        raise RuntimeError(f"{path}: invalid JSON ({exc})") from exc


def commitment_age_seconds(c: dict[str, Any], now: float) -> float:
    ts = c.get("submitted_at_unix")
    if isinstance(ts, (int, float)):
        return max(0.0, now - float(ts))
    return float("inf")


def latest_commitment(state: dict[str, Any], source_chain: str) -> dict[str, Any] | None:
    commits = state.get("commitments", [])
    filtered = [c for c in commits if c.get("source_chain") == source_chain]
    if not filtered:
        return None
    return max(filtered, key=lambda c: int(c.get("height", 0)))


def check_once(args: argparse.Namespace) -> int:
    now = time.time()
    l1 = load_json(pathlib.Path(args.l1_state))
    l2 = load_json(pathlib.Path(args.l2_state))

    l2_from_l3 = latest_commitment(l2, "OBOLOS")
    l1_from_l2 = latest_commitment(l1, "DRACHMA")

    issues: list[str] = []

    if l2_from_l3 is None:
        issues.append("L2 missing OBOLOS commitment")
    else:
        age = commitment_age_seconds(l2_from_l3, now)
        if age > args.freshness_seconds:
            issues.append(
                f"L2 latest OBOLOS checkpoint stale ({age:.0f}s > {args.freshness_seconds}s)"
            )

    if l1_from_l2 is None:
        issues.append("L1 missing DRACHMA commitment")
    else:
        age = commitment_age_seconds(l1_from_l2, now)
        if age > args.freshness_seconds:
            issues.append(
                f"L1 latest DRACHMA checkpoint stale ({age:.0f}s > {args.freshness_seconds}s)"
            )

    if l1_from_l2 and l2_from_l3:
        refs = l1_from_l2.get("references", [])
        ob_hash = l2_from_l3.get("obolos_hash")
        if ob_hash and ob_hash not in refs:
            issues.append("Route mismatch: L1 DRACHMA checkpoint does not reference latest OBOLOS hash")

    if issues:
        print(f"{ALERT}: " + " | ".join(issues))
        return 1

    print(f"{OK}: checkpoint freshness and route consistency verified")
    return 0


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--l1-state", default=".devnet/state/l1.json")
    p.add_argument("--l2-state", default=".devnet/state/l2.json")
    p.add_argument("--freshness-seconds", type=int, default=300)
    p.add_argument("--interval-seconds", type=int, default=15)
    p.add_argument("--oneshot", action="store_true")
    args = p.parse_args()

    if args.oneshot:
        return check_once(args)

    while True:
        rc = check_once(args)
        if rc != 0:
            # daemon mode: continue to alert while unhealthy, don't exit.
            pass
        time.sleep(max(1, args.interval_seconds))


if __name__ == "__main__":
    sys.exit(main())
