#!/usr/bin/env python3
"""Detect adversarial multi-node conditions.

Checks:
- node liveness quorum for provided peer state files
- equivocation: same commitment id with different payloads across peers
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import sys
from typing import Any


def load(path: pathlib.Path) -> dict[str, Any] | None:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return None


def fingerprint(commitment: dict[str, Any]) -> str:
    canonical = json.dumps(commitment, sort_keys=True, separators=(",", ":")).encode()
    return hashlib.sha256(canonical).hexdigest()


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--states", nargs="+", required=True)
    p.add_argument("--min-live", type=int, default=3)
    args = p.parse_args()

    loaded: dict[str, dict[str, Any]] = {}
    for s in args.states:
        st = load(pathlib.Path(s))
        if st is not None:
            loaded[s] = st

    errors: list[str] = []
    if len(loaded) < args.min_live:
        errors.append(f"liveness degradation: live_nodes={len(loaded)} < min_live={args.min_live}")

    by_id: dict[str, set[str]] = {}
    for _, st in loaded.items():
        for c in st.get("commitments", []):
            cid = str(c.get("id", ""))
            if not cid:
                continue
            by_id.setdefault(cid, set()).add(fingerprint(c))

    equivocated = [cid for cid, fp in by_id.items() if len(fp) > 1]
    if equivocated:
        errors.append("equivocation detected for commitment ids: " + ",".join(sorted(equivocated)))

    if errors:
        print("ADVERSARIAL_ALERT: " + " | ".join(errors))
        return 1

    print("ADVERSARIAL_OK: no equivocation and liveness quorum satisfied")
    return 0


if __name__ == "__main__":
    sys.exit(main())
