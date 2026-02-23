# ParthenonChain Implementation Gap Audit

This document tracks production-readiness gaps and explicitly separates **implemented work** from
**remaining work**.

## Current status at a glance

ParthenonChain has improved correctness and validation in node startup, consensus wiring,
serialization, storage parsing, and RPC input handling. However, the project is still not fully
production-complete across all subsystems.

## Completed recently

- Node startup now reconstructs in-memory chainstate from persisted blocks and fails fast on
  inconsistency.
- Chainstate tracks/enforces tip linkage and exposes tip hash handling.
- Genesis helpers and expected-hash checks were added.
- Canonical/non-canonical parsing checks were improved in wire/protocol and block parsing paths.
- Storage and RPC parsing hardening was added with tests.
- Firmware verification now includes revocation/rotation and anti-rollback validation tests.

## High-value work remaining

### 1) Deterministic dependency/release gating

**Status:** In progress (now enforced with explicit CI gate + pinned gitlinks check).

What is now in place:
- `scripts/ci/verify_real_deps.sh` verifies required dependency submodules exist, are initialized,
  and are pinned to the exact gitlink commit.
- `.github/workflows/dependency-gate.yml` enforces deterministic submodule bootstrap and Release
  CMake configure with `-DPARTHENON_REQUIRE_REAL_DEPS=ON`.
- Vendored archives (if introduced) must be listed in `scripts/ci/vendored_archives.sha256` and
  are verified via SHA-256 during the dependency gate.

What still remains:
- Extend release pipeline to publish and verify an SBOM/provenance attestation.

### 2) Hardware crypto / wallet firmware placeholder replacement

**Status:** In progress.

What is now in place:
- `HardwareAES` no longer uses memcpy placeholder behavior; it now performs authenticated
  AES-256-GCM encryption/decryption with per-message random nonce and tag verification.

Remaining:
- Replace remaining placeholder/mock behavior in firmware trust/update paths with
  production-grade implementations.

### 3) Mobile SDK productionization

**Status:** Not complete.

Remaining:
- Replace dummy wallet derivation/signing/network/subscription/storage behavior with production
  implementations.
- Add end-to-end SDK tests against local/regtest infrastructure.

### 4) Layer2 proof/rollup hardening

**Status:** Not complete.

Remaining:
- Replace placeholder proving logic with integrated proof generation/verification backend.
- Add lifecycle tests (batching, challenge/fraud flows, finalization/exit paths).

### 5) Operational hardening

**Status:** Partially documented.

Remaining:
- Complete operator-grade lifecycle/recovery/deployment runbooks.
- Include backup/restore drills, key compromise handling, and release incident procedures.


### 6) Governance module hardening

**Status:** Partially implemented. Unit tests added.

What is now in place:
- `VotingSystem` manages the full proposal lifecycle (PENDING → ACTIVE → PASSED/REJECTED →
  EXECUTED/EXPIRED) with Schnorr-authenticated votes (BIP-340), quorum enforcement, configurable
  voting periods, and double-vote prevention.
- `TreasuryManager` tracks deposits/withdrawals; withdrawals require a non-zero proposal_id.
- `DelegationSystem` supports delegation and undelegation of voting power between addresses.
- `UpdateBlockHeight()` / `GetBlockHeight()` exposed on `VotingSystem` so the consensus layer
  (and tests) can advance the block counter.
- Comprehensive unit tests added at `tests/unit/governance/test_governance.cpp` covering all
  three subsystems.

Remaining:
- Persist proposals and votes to the storage layer (LevelDB/RocksDB) so state survives restarts.
- Wire `UpdateBlockHeight()` into the consensus layer so block height advances automatically.
- Implement actual proposal execution handlers per `ProposalType` (currently a no-op placeholder).
- Add RPC and CLI endpoints for proposal creation, voting, and status queries.
- Integrate voter eligibility check against the ledger (token balance / stake weight).
- Complete the enterprise `ConsortiumManager` (permissioned.h) and link it to `VotingSystem`.

## Definition of done for “production-ready”

All items below should be true before claiming full production readiness:

**Status:** Not complete.

Remaining:
- Replace placeholder/mock behavior in hardware crypto and firmware trust/update paths with
  production-grade implementations.

### 3) Mobile SDK productionization

**Status:** Not complete.

Remaining:
- Replace dummy wallet derivation/signing/network/subscription/storage behavior with production
  implementations.
- Add end-to-end SDK tests against local/regtest infrastructure.

### 4) Layer2 proof/rollup hardening

**Status:** Not complete.

Remaining:
- Replace placeholder proving logic with integrated proof generation/verification backend.
- Add lifecycle tests (batching, challenge/fraud flows, finalization/exit paths).

### 5) Operational hardening

**Status:** Partially documented.

Remaining:
- Complete operator-grade lifecycle/recovery/deployment runbooks.
- Include backup/restore drills, key compromise handling, and release incident procedures.

## Definition of done for “production-ready”

All items below should be true before claiming full production readiness:

1. Reproducible release build with real dependencies only and deterministic bootstrap.
2. No consensus/network-critical placeholder logic.
3. Full network profile invariants enforced at startup (genesis + magic + ports + seed profile).
4. End-to-end and adversarial tests for hardware security, mobile SDK behavior, and rollup proof
   lifecycle.
5. Operator playbooks validated in recovery drills.
