# Changelog

All notable changes to ParthenonChain will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-01-12

### Added

#### Phase 1: Cryptographic Primitives
- SHA-256 implementation (FIPS 180-4 compliant)
- SHA-256d (double SHA-256) for block hashing
- Tagged SHA-256 (BIP-340 style) for domain separation
- Schnorr signatures (BIP-340) on secp256k1
- Comprehensive cryptographic test suite

#### Phase 2: Primitives & Data Structures
- Block structure with header and transaction list
- Transaction format with multi-asset UTXO support
- Script system for transaction validation
- Merkle tree implementation for transaction commitments
- Serialization/deserialization utilities

#### Phase 3: Consensus & Issuance
- Proof-of-Work consensus (SHA-256d)
- Difficulty adjustment algorithm
- Multi-asset issuance schedules:
  - TALANTON: 21M max supply
  - DRACHMA: 41M max supply
  - OBOLOS: 61M max supply
- Block reward calculation
- Halving schedules for each asset

#### Phase 4: Chainstate & Validation
- UTXO set management
- Chain state tracking
- Block validation logic
- Transaction validation
- Reorganization handling
- State root computation

#### Phase 5: Networking & Mempool
- P2P network protocol
- Peer discovery and management
- Block propagation
- Transaction relay
- Mempool with fee-based prioritization
- DoS protection

#### Phase 6: Smart Contracts (OBOLOS)
- EVM-compatible execution engine
- Opcode implementation (arithmetic, logic, storage, control flow)
- Gas metering and limits
- State management
- Contract deployment
- EIP-1559 style gas economics (base fee + priority tip)

#### Phase 7: DRM Settlement
- Multi-signature escrow
- Time-locked transfers
- Destination tags for payment routing
- Rights transfer primitives
- Escrow dispute resolution

#### Phase 8: Layer 2 Modules
- Payment channel implementation
- HTLC (Hash Time Locked Contracts)
- SPV bridge for cross-chain verification
- Transaction indexer
- Contract event indexer
- GraphQL API
- WebSocket API for real-time updates

#### Phase 9: Clients
- **parthenond**: Full node daemon
  - Block validation and storage
  - P2P networking
  - RPC server
  - Mining support
- **parthenon-cli**: Command-line interface
  - Wallet management
  - Transaction creation
  - Blockchain queries
- **Desktop GUI**: Qt-based graphical wallet
  - Transaction history
  - Address management
  - Network statistics
- **Mobile Wallet**: React Native implementation
  - iOS and Android support
  - SPV mode
  - QR code scanning
  - Share-mining module

#### Phase 10: Installers & Releases
- Windows NSIS installer (.exe)
- macOS DMG package (.dmg)
- Debian package (.deb)
- RPM package (.rpm)
- Checksum generation (SHA-256, SHA-512)
- GPG signature support
- Automated GitHub Actions CI/CD
- Release automation

### Security
- Deterministic execution throughout consensus layer
- Constant-time cryptographic operations
- Memory-safe key handling
- Input validation and sanitization
- Rate limiting and DoS protection

### Documentation
- Comprehensive architecture documentation
- API reference for all components
- Installation guides for all platforms
- Security model documentation
- Development guidelines

### Testing
- Unit tests for all core components
- Integration tests for multi-component workflows
- Consensus tests with edge cases
- Cryptographic test vectors
- Network simulation tests

## [0.1.0] - 2026-01-01 (Internal Development)

### Added
- Initial project structure
- Build system setup (CMake)
- Git repository initialization
- Development environment configuration

---

## Version History Summary

- **1.0.0** (2026-01-12): Production release - All 10 phases complete
- **0.1.0** (2026-01-01): Initial development setup

## Upgrade Notes

### Upgrading to 1.0.0

This is the first production release. There is no prior version to upgrade from.

For new installations, see [INSTALLATION.md](docs/INSTALLATION.md).

## Breaking Changes

None in this release (first production version).

## Deprecations

None in this release.

## Future Roadmap

Planned for future releases:

- Hardware wallet integration
- Lightning-style payment channels
- Cross-chain atomic swaps
- Advanced contract debugging tools
- Mobile share-mining optimization
- Built-in Tor support
- Multisig UI improvements
- External security audit

---

**Note**: For detailed information about each phase, see the `docs/PHASE*_COMPLETION.md` files.
