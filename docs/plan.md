# Layered Refactor Plan (Execution Order)

This plan reflects the canonical stack mandated for PantheonChain:

`OBOLOS (L3) -> DRACHMA (L2) -> TALANTON (L1)`

## Commit 1 — Shared core and commitment schema

- Harden `src/common` primitives (serialization, commitments, bridge checks, storage, mempool, metrics).
- Standardize commitment shape for `TX_L2_COMMIT` and `TX_L3_COMMIT`.
- Ensure commitment decoding rejects malformed payloads deterministically.

## Commit 2 — TALANTON as PoW security anchor

- Keep SHA-256d PoW behavior untouched.
- Enforce strict `TX_L2_COMMIT` validation:
  - source chain must be DRACHMA
  - finalized height monotonicity
  - payload encoding checks
  - >=2/3 active-stake signatures

## Commit 3 — DRACHMA L2 PoS payments layer

- Implement deterministic proposer rotation and total active stake accounting.
- Implement slashing handlers for double-sign and equivocation.
- Add dedicated payments state machine for high-throughput transfers and fee accounting.
- Validate `TX_L3_COMMIT` from OBOLOS.

## Commit 4 — OBOLOS L3 PoS + execution

- Implement OBOLOS proposer determinism and finality validation utilities.
- Keep EVM-like execution path and gas accounting in `src/obolos/execution.*`.
- Emit finalized commitment payloads consumable by DRACHMA.

## Commit 5 — Relayers and anchoring path

- Keep `pantheon-relayer-l3` for OBOLOS -> DRACHMA.
- Keep `pantheon-relayer-l2` for DRACHMA -> TALANTON.
- Validate end-to-end anchoring semantics in tests.

## Commit 6 — Interfaces and operator workflow

- Maintain `pantheon-node --layer=l1|l2|l3` runtime mode.
- Maintain layer-aware CLI commands:
  - `pantheon-cli stake deposit --layer=l2`
  - `pantheon-cli deploy-contract --layer=l3`
  - `pantheon-cli submit-commitment --layer=l2|l3`

## Commit 7 — Validation, CI, and docs

- Expand unit tests for staking/slashing, proposer determinism, commitments, and bridge flows.
- Add fuzzing around commitment decoding and proposer selection inputs.
- Keep docs synchronized: `architecture.md`, `consensus.md`, `threat-model.md`, `migration.md`, `plan.md`.
