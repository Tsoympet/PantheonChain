# Changelog

All notable changes to PantheonChain will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
