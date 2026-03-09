# Changelog

All notable changes to PantheonChain will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.3.0] - 2026-03-09

### Added

#### Build System
- CMakeLists.txt now reads version from `VERSION.txt` (was hardcoded `1.0.0`).
- Added `install()` rules for all five binaries, configs, genesis files, and docs.
- Added CPack configuration for DEB and RPM package generation.

#### Network Bootstrap
- Added `p2p_bootstrap_nodes` and `dns_seeds` arrays to all testnet and mainnet JSON configs
  (`configs/testnet/l1.json`, `l2.json`, `l3.json`, `configs/mainnet/l1.json`, `l2.json`, `l3.json`).
- Updated `configs/config.schema.json` with full schema for new fields, including a
  conditional rule requiring ≥5 bootstrap nodes and ≥1 DNS seed on mainnet.

#### Prometheus Metrics
- Expanded `MetricsRegistry` (`common/metrics/`) with gauges, histograms, labeled counters,
  `ScopedTimer` RAII helper, and `PrometheusText()` Prometheus v0.0.4 exposition format.
- Added `/metrics` HTTP endpoint to `RPCServer` exposing live block height, peer count,
  sync status, and per-method RPC request count / latency histogram.
- Enriched `/health` response with `best_height`, `peer_count`, and `syncing` fields.

#### Deployment
- Added five systemd unit files under `installers/linux/systemd/`:
  `pantheon-l1.service`, `pantheon-l2.service`, `pantheon-l3.service`,
  `pantheon-relayer-l2.service`, `pantheon-relayer-l3.service`.
  Each unit includes security hardening: `NoNewPrivileges`, `CapabilityBoundingSet=`,
  `ProtectSystem=strict`, memory and CPU limits.
- Added `docker/docker-compose.testnet.yml` with bootstrap peer config, fixed subnets,
  structured JSON logging (`json-file` driver, 7-day rotation), resource limits, and
  security context for each service.
- Upgraded `docker/docker-compose.yml` (mainnet) with the same hardening.

#### CLI Commands
- Added `wallet create|import|export|list|balance` subcommand group.
- Added `account balance|nonce|txs` subcommand group.
- Added `staking deposit|withdraw|rewards|status` subcommand group.
- Added `governance propose|vote|tally|list|get|execute` subcommand group.
- Added `node sync-status|peer-info|stop` subcommand group.
- Added `rpc call` for raw JSON-RPC invocation.
- Added `config validate` delegating to `scripts/validate-config.py`.
- Added `--version` flag.
- Improved output prefix: `[layer] action: detail` instead of raw `action accepted on layer`.

#### Operations
- Added `docs/ops/mainnet_launch_checklist.md` — four-phase launch checklist with
  code/security, config, infrastructure, validator-set, dry-run, and launch-day steps.
- Added `docs/ops/validator_runbook.md` — complete validator lifecycle guide:
  setup, staking, key rotation, backup/restore, incident response, reward claiming, exit.
- Added `scripts/setup-validator.sh` — interactive wizard that generates a secp256k1
  keypair, prints staking and key-import commands, and enforces key file permissions.
- Added `scripts/run-mainnet.sh` — production launch script with pre-flight checks,
  ordered startup (L1→L2→L3), relayer launch, checkpoint watchdog, and graceful shutdown.

#### Security
- Rewrote `SECURITY.md` with severity classification table (Critical/High/Medium/Low/Info),
  response SLA table, in-scope component matrix, hardening checklist, and known limitations.

### Fixed

- DEB control file: package name `parthenon` → `pantheonchain`, version `1.0.0` → `2.2.0`,
  removed Qt dependencies (not required for node binaries), correct maintainer contact.
- RPM spec file: same name/version corrections, updated `%files` for all five binaries
  and systemd units, replaced `parthenon` user with `pantheon`.
- `build-deb.sh`: reads version from `VERSION.txt` instead of hardcoded `1.0.0`.


### Changed

#### Consensus Model Clarification
- Rewrote architecture, consensus, threat, security, and whitepaper documentation to formalize PantheonChain as a layered hybrid model: TALANTON (L1 PoW settlement), DRACHMA (L2 PoS/BFT payments), and OBOLOS (L3 PoS/BFT EVM execution).
- Added `docs/SETTLEMENT_AND_FINALITY.md` with explicit finality semantics, trust assumptions, and failure scenarios for checkpointed settlement (`OBOLOS -> DRACHMA -> TALANTON`).
- Added `scripts/validate-layer-model.py` and updated configuration docs to enforce role naming and checkpoint cadence consistency across L1/L2/L3 config profiles.
- Added canonical machine-readable layer metadata in `configs/layer-model.json` and refactored `scripts/validate-layer-model.py` to validate against it as the single source of truth.
- Added a CI architecture-consistency gate in `.github/workflows/test.yml` to run layered config and model checks before platform test matrices.
- Expanded RPC documentation with canonical commitment payload field names and validator/stake terminology guidance.
- Added `docs/ARCHITECTURE_ALIGNMENT_GAPS.md` to track unresolved ambiguities, potential code/docs divergence points, and prioritized next implementation steps.

#### Architecture Alignment Gaps — Code-Level Improvements
- **Validator/stake terminology** is now canonical throughout all L2/L3 code and documentation:
  - `struct Validator` (with `stake` field) is the primary type in `layer2-drachma` and `layer3-obolos` consensus headers; the PoW-era `Miner`/`hash_power` names are fully removed.
  - `TotalActiveStake()` and `SelectDeterministicProposer()` are now the sole canonical function names; `TotalHashPower()` and `SelectMiner()` are removed.
  - `FinalitySignature::stake` replaces the former `hash_power` field in `common/serialization/commitments.h`.
  - `active_stake` replaces `active_pow` in `ValidateL3Commit` and `ValidateL3Finality` signatures.
- **Relayer CLI** (`pantheon-relayer-l3`): removed legacy `--active-pow` flag. Use `--active-stake`.
- **EVM block context** (`layer3-obolos/evm`): renamed "Block miner" / "Priority fee goes to miner" comments to "Block proposer" throughout, consistent with PoS/BFT execution layer.
- **Block explorer** (`tools/block_explorer`): renamed `BlockInfo::miner` field to `proposer`; updated GraphQL schema in `docs/LAYER2_PROTOCOLS.md` accordingly.
- **Policy metadata**: extended `configs/layer-model.json` with `policy` object (`l1_min_confirmation_depth`, `bridge_unlock_min_l1_depth`, `checkpoint_freshness_slo_seconds`, `relayer_liveness_threshold_seconds`); added corresponding fields to all operator `l1.json` configs (devnet/testnet/mainnet).
- **Terminology lint in CI**: added `scripts/lint-docs-terminology.py` and a `docs-terminology-lint` job in `.github/workflows/lint.yml` to reject known deprecated phrases in consensus-critical docs on every pull request.
- **Operations docs**: updated `docs/OPERATIONS_RUNBOOK.md` to distinguish L1 miners from L2/L3 validators in maintenance procedures.

---

## [2.2.0] - 2026-03-06

### Changed

#### Governance — One-Address-One-Vote (1A1V)
- **Voting model**: Replaced balance-proportional voting with **one-address-one-vote (1A1V)**.
  Every address holding any token balance gets exactly **1 vote** regardless of how many
  tokens it holds. A holder with 1 TALN and a holder with 1 billion DRM cast votes of
  identical weight.
- `BalanceVotingRegistry::GetVotingPower()` now returns `1` for any holder, `0` otherwise
  (was: token balance amount).
- `BalanceVotingRegistry::GetTotalVotingPower()` now returns the count of eligible voters
  (was: sum of all balances).
- `BalanceVotingRegistry::GetAllVotingPowers()` now returns `(address, 1)` for each holder
  (was: `(address, balance)`).
- `GovernanceParams::quadratic_voting_enabled` default changed to `false` (was `true`).
- `AntiWhaleGuard::kDefaultConfig.quadratic_voting_enabled` changed to `false`.
  The guard is retained as a utility class but has no effect on live votes because
  snapshot power is always 1.
- `staking/get_power` RPC endpoint now returns `source: "one_address_one_vote"` and
  `total_voters` (count of eligible voters) instead of `total_power` (sum of balances).
- All docs (`CONSTITUTION.md`, `WHITEPAPER.md`, `consensus.md`, runbooks, desktop UI,
  mobile UI) updated to reflect 1A1V semantics.

---

## [2.1.0] - 2026-01-20

### Added

#### Governance
- VRF sortition for Boule (council) candidate selection — on-chain verifiable randomness for fair validator rotation
- Snapshot voting: voting power is captured at proposal-creation block height to prevent flash-stake manipulation
- Vesting grants: Treasury can issue milestone-based token grants with cliff + linear vesting schedules
- Per-type execution dispatch: STANDARD, CONSTITUTIONAL, EMERGENCY, PARAMETER_CHANGE, and TREASURY_SPENDING proposals each have independent execution paths and threshold rules
- `Apophasis` board: ratification body for Emergency Council actions to ensure post-hoc accountability

#### RPC
- 12 new JSON-RPC endpoints covering governance, staking, treasury, and ostracism:
  - `governance_getProposal`, `governance_listProposals`, `governance_castVote`
  - `governance_submitProposal`, `governance_executeProposal`, `governance_getVoteRecord`
  - `staking_getValidator`, `staking_listValidators`, `staking_getDelegation`
  - `treasury_getBalance`, `treasury_listGrants`, `ostracism_getStatus`

#### Layer 2 / DRACHMA
- Positional Merkle tree: O(log n) membership proofs for Plasma exit verification
- Plasma `ChallengeExit` verification: on-chain fraud proof handler for invalid exit claims

#### Operations
- SBOM (Software Bill of Materials) generation in CI pipeline (CycloneDX format)
- Build provenance attestation via SLSA Level 2 CI workflow
- Runbooks added: `backup-restore.md`, `key-compromise.md`, `incident-response.md`

#### Documentation
- `docs/CONSTITUTION.md`: full on-chain Governance Constitution with ancient Athenian democratic principles
- Removed internal planning and audit documents from repository

### Fixed

#### CI
- Resolved `clang-format` violations across `layer2-drachma/` and `layer3-obolos/` source trees
- Fixed GCC `-Wcomment` warning caused by nested comment sequences (`*/` inside block comments)
- Lint and Security CI workflows now passing

---

## [1.0.0] - 2026-01-13

### Added

#### Core Blockchain (Complete)
- SHA-256 implementation (FIPS 180-4 compliant)
- SHA-256d (double SHA-256) for block hashing
- Tagged SHA-256 (BIP-340 style) for domain separation
- Schnorr signatures (BIP-340) on secp256k1
- Block structure with header and transaction list
- Transaction format with multi-asset UTXO support
- Script system for transaction validation
- Merkle tree implementation for transaction commitments
- Serialization/deserialization utilities

#### Consensus & Validation (Complete)
- Proof-of-Work consensus (SHA-256d)
- Difficulty adjustment algorithm
- Multi-asset issuance schedules:
  - TALANTON: 21M max supply
  - DRACHMA: 41M max supply
  - OBOLOS: 61M max supply
- Block reward calculation
- Halving schedules for each asset
- UTXO set management
- Chain state tracking
- Block validation logic
- Transaction validation
- Reorganization handling
- State root computation

#### Networking & Mining (Complete)
- P2P network protocol with TCP socket implementation
- Peer management
- Block propagation
- Transaction relay
- Mempool with fee-based prioritization
- DoS protection
- Block mining with full integration
- Mining difficulty adjustment

#### Smart Contracts - OBOLOS (Complete)
- EVM-compatible execution engine
- Full 256-bit arithmetic implementation
- Opcode implementation (arithmetic, logic, storage, control flow)
- Gas metering and limits
- State management with Merkle Patricia Trie
- Contract deployment and interaction
- EIP-1559 style gas economics (base fee + priority tip)

#### DRM Settlement (Complete)
- Multi-signature escrow
- Time-locked transfers
- Destination tags for payment routing
- Rights transfer primitives

#### RPC & Wallet (Complete)
- RPC server with method implementations
- JSON-RPC protocol support
- Wallet with HD key derivation
- UTXO tracking and synchronization
- Balance management for all assets
- Transaction creation and signing
### In Development

#### Optional Performance Optimizations
- GPU acceleration for signature verification (CUDA)
- DPDK zero-copy networking

#### Future Enhancements (Planned)
- Layer 2 payment channels
- HTLC (Hash Time Locked Contracts) for atomic swaps
- SPV light client support
- Transaction and contract indexers
- GraphQL API
- WebSocket API for real-time updates
- Desktop GUI (Qt-based)
- Mobile wallet applications

### Testing
- Unit tests for all core components (21/21 passing)
- Integration tests for multi-component workflows
- Consensus tests with edge cases
- Cryptographic test vectors

### Documentation
- Comprehensive README with accurate status
- Architecture documentation
- Security policy and best practices
- Technical whitepaper
- End-user license agreement (EULA)
- Changelog with version history

## [0.1.0] - 2026-01-01 (Internal Development)

### Added
- Initial project structure
- Build system setup (CMake)
- Git repository initialization
- Development environment configuration

---

## Version History Summary

- **2.1.0** (2026-01-20): Governance VRF sortition, 12 new RPC endpoints, Layer 2 Plasma improvements, SBOM/provenance CI, runbooks, Constitution
- **1.0.0** (2026-01-13): Core blockchain complete, ready for testnet
- **0.1.0** (2026-01-01): Initial development setup

## Upgrade Notes

### Upgrading to 1.0.0

This is the first public release with core blockchain functionality; production readiness work remains in progress.

For new installations, see [INSTALLATION.md](docs/INSTALLATION.md).

## Breaking Changes

None in this release (first production version).

## Deprecations

None in this release.

## Future Roadmap

### Short-term (Next Release)
- HTTP server backend for RPC (using cpp-httplib or similar)
- Performance benchmarking and optimization

### Medium-term
- Hardware wallet integration
- Transaction and contract indexers with database backend
- GraphQL and WebSocket APIs
- SPV light client implementation

### Long-term
- Lightning-style payment channels
- Cross-chain atomic swaps (HTLC)
- Desktop GUI application
- Mobile wallet applications
- Advanced contract debugging tools
- GPU/DPDK performance optimizations
- Built-in Tor support
- External security audit

---

**Note**: For detailed information about each phase, see the `docs/PHASE*_COMPLETION.md` files.
