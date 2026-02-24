# Rebuild Absorption Plan

## Overview

This document describes the plan for absorbing PantheonChain rebuild changes back
into the main development branch, ensuring build stability and test coverage across
all three layers (Talanton L1, Drachma L2, Obolos L3).

## Scope

The rebuild absorption covers:

- Layer 1 (Talanton): consensus engine, ledger, transaction processing, RPC, and genesis state.
- Layer 2 (Drachma): staking, payments, L1 bridge, and cross-layer relayer.
- Layer 3 (Obolos): EVM compatibility, gas accounting, L2 bridge, and relayer.
- Common libraries: crypto primitives, P2P networking, storage, serialization, mempool, metrics.
- CLI tooling (`pantheon-cli`) and Docker infrastructure.

## Steps

1. **Merge feature branches** — cherry-pick or merge individual layer branches into
   `develop` sequentially: `layer1`, `layer2`, `layer3`, `common`, `relayers`, `cli`.
2. **Run repository audit** — execute `./scripts/repo-audit.sh` to verify all required
   directories, scripts, and documentation are present.
3. **Build verification** — execute `./scripts/build.sh` on a clean workspace and confirm
   zero compilation errors on Linux (Ubuntu 24.04) and macOS.
4. **Unit tests** — run `ctest --test-dir build --output-on-failure`; all tests must pass.
5. **Integration smoke** — run `./scripts/run-devnet.sh` followed by
   `./tests/integration/devnet-smoke.sh`; all RPC health checks and commitment-anchoring
   assertions must pass.
6. **Placeholder gate** — run `./scripts/placeholder-gate.sh`; no forbidden markers
   (`TODO(PROD)`, `PLACEHOLDER`, `DUMMY`, `MOCK`, `FIXME(PROD)`) must be present in
   production paths.
7. **Lint** — run `./scripts/lint.sh`; no clang-format violations.
8. **Mobile CI** — verify Mobile Android and Mobile iOS installer workflows pass on
   `main`.
9. **Tag release** — after all gates pass, tag the commit as `v<major>.<minor>.<patch>`.

## Rollback Criteria

If any of the following occur during absorption, revert the offending merge and open
a tracking issue:

- `repo-audit.sh` fails on a newly required file or directory.
- `build.sh` exits non-zero on the CI runner image.
- More than 1% of unit tests regress.
- Integration smoke fails end-to-end commitment anchoring.

## Owners

| Area     | Owner          |
|----------|----------------|
| L1       | Core Team      |
| L2 / L3  | Protocol Team  |
| CLI      | Tooling Team   |
| CI / CD  | DevOps         |
