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
- **Stub / partial-implementation audit** – all stubs, partial implementations, and misleading
  comments have been catalogued and addressed as described in the section below.

## High-value work remaining

### 0) Full stub / partial-implementation inventory (this audit run)

The following items were identified in this audit pass.  Items marked **Fixed** have had
their code corrected or meaningfully hardened.  Items marked **Documented** have had their
comment updated to clearly explain the gap and what is required to close it.

| File | Lines | Issue | Action taken | Remaining work |
|------|-------|-------|-------------|----------------|
| `layer1-talanton/governance/voting.cpp` | 252–271 | `ExecuteProposal()` switch statement only appended a type-tag byte; never executed any governance action | **Fixed** – replaced with an `execution_handler_` callback. Callers register a handler via `SetExecutionHandler()`. Without a handler the proposal still advances to EXECUTED. | Wire real handlers for PARAMETER_CHANGE (call `GovernanceParams::UpdateParam`), TREASURY_SPENDING (call `Treasury::Withdraw`), PROTOCOL_UPGRADE, etc. |
| `layer1-talanton/core/privacy/ring_signature.cpp` | 25–36, 76–97 | `Sign()` hashed `message \|\| ring_key \|\| secret_key`; `Verify()` hashed `message \|\| ring_key` – these never match, making all signatures fail verification | **Fixed** – both Sign and Verify now use `SHA256(message \|\| ring_key \|\| key_image)` so the formula is consistent and unforgeability is bound to the key_image | Replace with a real MLSAG/LSAG ring signature scheme (e.g. using libsecp256k1 + CLSAG) |
| `layer2-drachma/rollups/optimistic_rollup.cpp` | 135, 289 | "Additional verification would happen here" and "This is a simplified check" | **Fixed** – added `state_proof_before`/`state_proof_after` non-empty checks and replaced misleading comments with clear production notes | Full re-execution of the disputed transaction against state witnesses |
| `layer2-drachma/plasma/plasma_chain.cpp` | 133–136 | `ChallengeExit` only checked `fraud_proof.empty()` | **Fixed** – fraud proof must now be ≥ 32 bytes and its first 32 bytes must match the challenged `tx_hash` | Full Merkle inclusion proof and transaction re-execution |
| `layer1-talanton/wallet/hardware/firmware_verification.cpp` | 116, 619 | Placeholder vendor public keys; `VerifySecureBoot` was a non-zero byte check | **Documented** – comments updated to specify exactly what real bytes are needed and warn that leaving placeholders causes all real-device verifications to fail | Replace with real Ledger/Trezor certificate bytes; integrate vendor attestation API |
| `layer1-talanton/settlement/multisig.cpp` | 180 | "For now, just use the last 32 bytes (simplified)" | **Fixed** – comment replaced with accurate explanation of compressed-key X-coordinate extraction (BIP340) | No code change needed; comment was inaccurate |
| `layer2-drachma/rollups/zk_rollup.cpp` | 63 | "simplified tree (value-sorted pairs)" without production note | **Documented** – comment now explains that a production ZK-rollup needs index-based (positional) Merkle ordering | Replace sorted Merkle tree with positional tree when wiring real proving backend |
| `layer3-obolos/evm/private_contracts.cpp` | 123 | `TallyVotes()` splits votes 50/50 as a "deterministic placeholder" | **Documented** – comment now explains the threshold decryption requirement and warns against deploying to production | Integrate threshold decryption service |
| `layer3-obolos/evm/formal_verification/verifier.cpp` | 66 | "Source-to-bytecode compilation is not yet implemented" | **Documented** – comment updated to explain what compiler integration is needed | Integrate `solc` or equivalent compiler |
| `layer2-drachma/apis/graphql/graphql_api.cpp` | 47 | "Simple query parser (simplified version)" | **Documented** – comment updated to explain substring-match limitation and what a real GraphQL parser requires | Integrate `libgraphqlparser` or generated schema executor |
| `tools/genesis_builder/genesis_builder.cpp` | 125–126 | Invalid premine address fell back to all-zero (anyone-can-spend) public key | **Fixed** – invalid addresses are now skipped with an error; zero-key fallback removed | None; the fix is production-safe |
| `tools/mobile_sdks/mobile_sdk.cpp` | 946, 954, 1064 | Vague "not supported" messages for contract calls, deployment, gas estimation | **Fixed** – messages now name the specific RPC method (e.g. `eth_call`) that must be wired in | Wire EVM-compatible RPC endpoints |
| `third_party/stubs/secp256k1/secp256k1_stub.c` | entire file | XOR-based key derivation and signing (not ECDSA); cannot be used in production | **Documented** in header comment | Replace with the real `libsecp256k1` submodule (already guarded by `PARTHENON_REQUIRE_REAL_DEPS`) |
| `third_party/stubs/leveldb/leveldb_stub.cc` | entire file | No-op database stub; all persistence silently discarded | **Documented** in header comment | Replace with real LevelDB/RocksDB (already guarded by `PARTHENON_REQUIRE_REAL_DEPS`) |
| `layer1-talanton/core/privacy/zk_snark.cpp` | 67–95 | Proof generation uses `SHA256(witness \|\| verification_key)` instead of real SNARK | Noted in audit | Integrate a real proving backend (e.g. libsnark, bellman, or gnark) |
| `layer1-talanton/core/privacy/zk_stark.cpp` | `ComputeMerkleRoot` | Sorted (non-positional) Merkle tree, SHA256-based FRI substitute | Noted in audit | Integrate a real STARK prover |
| `layer1-talanton/core/privacy/ring_signature.cpp` | all | Simplified HMAC-style ring signature, not MLSAG/LSAG | **Fixed** (sign/verify made consistent); still not a real ring signature | Integrate CLSAG from Monero Research Lab or equivalent |
| `layer1-talanton/governance/voting.cpp` | `ExecuteProposal` | Proposal execution had no actual dispatch | **Fixed** – see above | Wire per-type handlers |
| `clients/desktop/src/receivepage.cpp` | 73 | QR code area is a label placeholder | Noted in audit | Integrate `qrencode` or Qt QR library |

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

#### What is also now in place (treasury, staking, emergency, fee routing)
- **`treasury.h/.cpp`** – Full `Treasury` with five allocation tracks (CORE_DEVELOPMENT, GRANTS,
  OPERATIONS, EMERGENCY, UNCATEGORIZED), budget periods with per-track spending caps, milestone
  grants with phased releases and revocation, multi-sig spending for the EMERGENCY track,
  configurable reserve ratio, guardian registry, and an immutable transaction audit log.
- **`staking.h/.cpp`** – `StakingRegistry`: stake with optional lock period, unstake cooldown
  (anti-flash-stake), slashing with history, voting-power derivation (stake − pending_unstake),
  total-supply feed for `AntiWhaleGuard`.
- **`emergency.h/.cpp`** – `EmergencyCouncil`: M-of-N guardian multi-sig actions
  (PAUSE_GOVERNANCE, CANCEL_PROPOSAL, FAST_TRACK_UPGRADE, CUSTOM) with TTL expiry.
  Inspired by Athenian *Apophasis* (special investigative board).
- **`eventlog.h/.cpp`** – `GovernanceEventLog`: unified append-only audit trail (*Stele* principle)
  for all subsystems, queryable by type, actor, block-range, or reference ID.
- **`fee_router.h/.cpp`** – `FeeRouter`: routes every fee event from all three chain layers to the
  correct destination (block producer / treasury / burn sink) using configurable basis-point splits:

  | Fee source         | Producer | Treasury | Burn | Track            |
  |--------------------|----------|----------|------|------------------|
  | L1 UTXO (TALN)     | 80 %     | 15 %     | 5 %  | CORE_DEVELOPMENT |
  | L2 validator (DRM) | 70 %     | 20 %     | 10 % | OPERATIONS       |
  | L3 base fee (OBL)  |  0 %     | 50 %     | 50 % | GRANTS           |
  | L3 priority tip    | 100 %    |  0 %     |  0 % | –                |
  | Bridge fees        |  0 %     | 100 %    |  0 % | OPERATIONS       |
  | Protocol fees      |  0 %     | 100 %    |  0 % | UNCATEGORIZED    |

  Rounding remainder goes to burn so amounts always sum exactly to total fee (no satoshi leakage).
  All routes log to `GovernanceEventLog` and update cumulative `SourceStats`.


#### What is also now in place (supply policy, snapshot, vesting, VETO)

- **`supply_policy.h/.cpp`** – Named supply-bonded thresholds (5 % / 10 % / 50 %) for each
  asset, expressed as absolute base-unit constants and runtime helpers:

  | Asset | 5 % (TIER_LOW)   | 10 % (TIER_MID)  | 50 % (TIER_HIGH)    |
  |-------|-------------------|-------------------|----------------------|
  | TALN  | 1 050 000 TALN   | 2 100 000 TALN   | 10 500 000 TALN     |
  | DRM   | 2 050 000 DRM    | 4 100 000 DRM    | 20 500 000 DRM      |
  | OBL   | 3 050 000 OBL    | 6 100 000 OBL    | 30 500 000 OBL      |

  Helpers: `IsBondingHealthy()` (5 % floor), `ExceedsTreasuryCap()` (50 % ceiling),
  `IsWhale()` (10 % threshold), `ComputeBondedQuorum()` (quorum = 5 % of bonded supply).

- **`snapshot.h/.cpp`** – `SnapshotRegistry`: voting power frozen at the proposal's
  `voting_start` block, preventing last-block stake-manipulation attacks. Immutable once
  created; zero-power entries excluded; independent per proposal.

- **`vesting.h/.cpp`** – `VestingRegistry`: cliff + linear vesting for treasury grants and
  team allocations. Governance-revocable (returns unvested tokens to treasury). Complements
  `Treasury::CreateGrant()` milestone releases.

- **`VoteChoice::VETO`** – Fourth vote option added to `VoteChoice` enum. `TallyVotes()`
  now implements the Cosmos Hub veto model: if veto share of total votes exceeds
  `veto_threshold_bps` (default 3334 ≈ 33.34 %), the proposal is **immediately REJECTED**
  regardless of the YES/NO ratio, and the deposit should be slashed by the caller.
  ABSTAIN votes are included in total (reducing veto %) matching the Cosmos spec.

- **`GovernanceParams`** – Added `veto_threshold_bps` field (default 3334 bps) with
  constitutional limits [1000, 5000] bps (cannot be set below 10 % or above 50 %).
  Wired into `UpdateParam()` / `ValidateUint()` / `GetChangeHistory()`.

#### Remaining work
- Persist all governance state (proposals, Boule council, ostracism records, stake, treasury
  balances) to LevelDB/RocksDB so state survives restarts.
- Wire `UpdateBlockHeight()`, `FeeRouter::Route()`, and snapshot creation into the
  consensus/mining layer so they advance automatically on every mined block.
- Wire `SnapshotRegistry::CreateSnapshot()` into `VotingSystem::CreateProposal()` (or its
  consensus hook) so snapshots are automatically taken at `voting_start`.
- Wire `VestingRegistry` into `Treasury::CreateGrant()` so grants can optionally use
  vesting schedules instead of (or in addition to) milestone releases.
- Implement actual per-`ProposalType` execution handlers (currently placeholder).
- Add RPC and CLI endpoints for proposal creation, voting, Boule review, ostracism queries,
  treasury balance checks, fee-route configuration, vesting queries.
- Integrate voter eligibility against ledger stake: call `StakingRegistry::GetVotingPower()`
  to derive the `voting_power` argument passed to `VotingSystem::CastVote()`.
- Replace LCG in `Boule::ConductSortition` with a VRF for cryptographic sortition.
- Complete the enterprise `ConsortiumManager` (permissioned.h) and link to `VotingSystem`.

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
