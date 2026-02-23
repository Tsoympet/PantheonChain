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


### 6) Governance module – ancient-Greece model + anti-whale

**Status:** Core complete. Unit tests added (46 tests across 6 suites).

#### Ancient-Greece analogy → blockchain mapping

| Ancient Athens              | PantheonChain module / method                     |
|-----------------------------|---------------------------------------------------|
| Kleroterion (lot machine)   | `Boule::ConductSortition()` – VRF-style draw      |
| Boule (Council of 500)      | `Boule` class – randomly-selected review council  |
| Ekklesia (Assembly)         | `VotingSystem` – all stakers vote on proposals    |
| Prytany (exec sub-committee)| `Boule::GetPrytany()` – rotating fast-track panel |
| Graphe Paranomon (challenge)| `Boule::RaiseGrapheParanomon()` – veto mechanism  |
| Dokimasia (eligibility)     | `Boule::RegisterCitizen()` + min-stake gate       |
| Ostracism (temporary ban)   | `Ostracism` class – community ban on bad actors   |
| Isonomia (rule of law)      | `GovernanceParams::kLimits` – constitutional floors|
| Isegoria (equal voice)      | Proposal deposit (accessible, not prohibitive)    |

#### What is now in place
- **`antiwhale.h/.cpp`** – `AntiWhaleGuard`: quadratic voting (floor(sqrt)), per-voter hard cap,
  whale-threshold detection; plugged into `VotingSystem::CastVote()`.
- **`boule.h/.cpp`** – `Boule`: citizen registry, sortition (Kleroterion), Dokimasia eligibility
  screening, 2/3-majority proposal review, Graphe Paranomon (unconstitutionality challenge with
  council vote resolution), Prytany (rotating executive committee).
- **`ostracism.h/.cpp`** – `Ostracism`: nominate bad actors, community vote, finalized ban,
  time-limited with automatic rehabilitation path.
- **`params.h/.cpp`** – `GovernanceParams`: all on-chain governance configuration, proposal-gated
  updates (proposal_id required), constitutional floors/ceilings (Isonomia), immutable change
  history, `Defaults()` function.
- **`voting.h/.cpp`** (extended) – `CONSTITUTIONAL` proposal type (66% threshold, Isonomia),
  `EMERGENCY` proposal type (Prytany fast-track); proposal deposit fields + `ReturnDeposit` /
  `SlashDeposit`; `SetAntiWhaleGuard()`, `SetBoule()`, `SetRequireBouleApproval()`,
  `SetTotalSupply()` integration hooks; `MarkBouleApproved()`.

#### Remaining work
- Persist proposals, Boule council, and ostracism records to LevelDB/RocksDB.
- Wire `UpdateBlockHeight()` into the consensus layer so it advances automatically.
- Implement actual per-`ProposalType` execution handlers (currently placeholder).
- Add RPC and CLI endpoints for proposal creation, voting, Boule review, and ostracism queries.
- Integrate voter eligibility against ledger stake (token balance → raw voting_power).
- Complete the enterprise `ConsortiumManager` (permissioned.h) and link to `VotingSystem`.
- Replace LCG in `Boule::ConductSortition` with a VRF for cryptographic sortition.

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
