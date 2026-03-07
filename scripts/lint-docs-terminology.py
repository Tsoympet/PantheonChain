#!/usr/bin/env python3
"""Lint consensus-critical documentation for deprecated PoW-era terminology.

Scans Markdown files in consensus, security, and architecture doc paths and
rejects any that contain known deprecated phrases.  Scoped to a small set of
files to reduce false positives on non-consensus prose.

Exit codes:
  0 — no violations found
  1 — one or more violations found
"""

from __future__ import annotations

import pathlib
import sys
from typing import Sequence

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# Directories (relative to repo root) to scan.
SCAN_DIRS: Sequence[str] = (
    "docs",
)

# Glob patterns within those directories.
SCAN_GLOB = "**/*.md"

# Deprecated phrases that must not appear in the scanned files.
# Each entry is (phrase_lower, human_readable_suggestion).
DEPRECATED_PHRASES: Sequence[tuple[str, str]] = (
    ("active_pow", "Use active_stake instead of active_pow"),
    ("--active-pow", "Use --active-stake instead of --active-pow"),
    ("total hash power", "Use 'total active stake' instead of 'total hash power'"),
    ("pow miners", "Use 'validators' instead of 'PoW miners'"),
    ("hash-power", "Use 'stake' instead of 'hash-power'"),
    ("miner_id", "Use 'validator_id' instead of 'miner_id'"),
    ("block producers are pow", "Use PoS/BFT block producer description"),
    ("replaces the former pos", "Do not reference PoS as the 'former' model; L2/L3 use PoS/BFT"),
)

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    repo_root = pathlib.Path(__file__).resolve().parent.parent
    violations: list[str] = []

    for scan_dir in SCAN_DIRS:
        base = repo_root / scan_dir
        if not base.is_dir():
            continue
        for path in sorted(base.glob(SCAN_GLOB)):
            try:
                text = path.read_text(encoding="utf-8")
            except OSError as exc:
                print(f"WARNING: cannot read {path}: {exc}", file=sys.stderr)
                continue

            text_lower = text.lower()
            for phrase, suggestion in DEPRECATED_PHRASES:
                if phrase in text_lower:
                    # Find line number for better diagnostics.
                    for lineno, line in enumerate(text.splitlines(), 1):
                        if phrase in line.lower():
                            violations.append(
                                f"{path.relative_to(repo_root)}:{lineno}: "
                                f"deprecated phrase '{phrase}' — {suggestion}"
                            )

    if violations:
        print("TERMINOLOGY LINT FAILURES:", file=sys.stderr)
        for v in violations:
            print(f"  {v}", file=sys.stderr)
        return 1

    print("Terminology lint passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
