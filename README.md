# ParthenonChain

A production-grade Proof-of-Work blockchain with multi-asset UTXO ledger, Schnorr signatures, and EVM-compatible smart contracts.

## Overview

ParthenonChain is a **Layer-1 blockchain** implementing a secure, deterministic consensus protocol with advanced features including:

- **SHA-256d Proof-of-Work** consensus (Bitcoin-compatible)
- **Multi-asset UTXO ledger** with three native tokens:
  - **TALANTON** (21M max supply)
  - **DRACHMA** (41M max supply)
  - **OBOLOS** (61M max supply)
- **Schnorr signatures** (BIP-340, secp256k1)
- **OBL EVM-compatible smart contracts** with deterministic execution
- **DRM settlement primitives** for digital rights management
- **Layer 2 protocols** (payment channels, HTLC bridges, indexers)

## Features

### Layer 1 (Consensus Critical)
- Deterministic block validation and state transitions
- Multi-asset issuance with fixed supply schedules
- Tagged SHA-256 for domain separation
- Full UTXO tracking and validation
- Integrated mempool with transaction prioritization
- P2P network with peer discovery and block propagation

### Smart Contracts (OBOLOS)
- EVM-compatible execution engine
- Gas economics with EIP-1559 style fee market
- Deterministic opcode execution
- State root computation
- Contract deployment and interaction

### DRM Settlement
- Multi-signature escrow
- Time-locked transfers
- Destination tags for payment routing
- Rights transfer primitives

### Layer 2
- Payment channels for instant micropayments
- HTLC bridges for cross-chain interoperability
- SPV verification
- Transaction and contract indexers
- GraphQL and WebSocket APIs

## Quick Start

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15 or higher
- OpenSSL development libraries
- Boost libraries (optional, for some components)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/Tsoympet/PantheonChain.git
cd PantheonChain

# Initialize submodules
git submodule update --init --recursive

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run tests
ctest
```

### Running a Full Node

```bash
# Start the daemon
./build/clients/core-daemon/parthenond

# Or with a custom config
./build/clients/core-daemon/parthenond /path/to/parthenond.conf
```

### Using the CLI

```bash
# Get blockchain info
./build/clients/cli/parthenon-cli getinfo

# Get wallet balance
./build/clients/cli/parthenon-cli getbalance

# Send TALANTON
./build/clients/cli/parthenon-cli send <address> <amount>
```

## Installation

Pre-built installers are available for all major platforms:

- **Windows**: [Download Installer](../../releases) (.exe)
- **macOS**: [Download DMG](../../releases) (.dmg)
- **Linux**: [Download DEB/RPM](../../releases) (.deb, .rpm)

For detailed installation instructions, see [INSTALLATION.md](docs/INSTALLATION.md).

## Documentation

- **[Whitepaper](WHITEPAPER.md)** - Technical whitepaper and system overview
- [Architecture Overview](docs/ARCHITECTURE.md) - System design and layer separation
- [Layer 1 Core](docs/LAYER1_CORE.md) - Consensus implementation details
- [Layer 2 Protocols](docs/LAYER2_PROTOCOLS.md) - Off-chain protocols
- [Installation Guide](docs/INSTALLATION.md) - Platform-specific setup
- [Security Model](docs/SECURITY_MODEL.md) - Security architecture
- [Genesis Block](docs/GENESIS.md) - Genesis configuration
- [Release Process](docs/RELEASES.md) - How releases are created
- [End-User License Agreement](EULA.md) - Software license terms

## Project Structure

```
ParthenonChain/
├── layer1/           # Consensus-critical code
│   ├── core/         # Cryptographic primitives, consensus, chainstate
│   ├── evm/          # OBOLOS smart contract engine
│   └── settlement/   # DRM settlement features
├── layer2/           # Non-consensus extensions
│   ├── payment_channels/
│   ├── bridges/
│   ├── indexers/
│   └── apis/
├── clients/          # End-user applications
│   ├── core-daemon/  # Full node (parthenond)
│   ├── cli/          # Command-line interface
│   ├── desktop/      # Desktop GUI (with icons)
│   └── mobile/       # Mobile wallet (with icons)
├── installers/       # Platform-specific installers
├── assets/           # SVG icons and branding
├── tests/            # Test suites
└── docs/             # Documentation
```

## Development

### Coding Standards

- **Language**: C++17
- **Build System**: CMake
- **Determinism**: No system randomness or time dependencies in consensus code
- **Layer Separation**: Strict boundary between Layer 1 and Layer 2
- **Testing**: Comprehensive unit and integration tests required

### Running Tests

```bash
# All tests
ctest

# Crypto tests only
ctest -R crypto

# Consensus tests
ctest -R consensus

# Verbose output
ctest -V
```

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

## Security

ParthenonChain takes security seriously. Please see [SECURITY.md](SECURITY.md) for:

- Security model overview
- Responsible disclosure policy
- Supported versions
- How to report vulnerabilities

## License

See [LICENSE](LICENSE) file for details.

## Acknowledgments

- Bitcoin Core for cryptographic primitives and consensus design inspiration
- Ethereum for EVM architecture
- BIP-340 authors for Schnorr signature standardization

## Contact

- GitHub: [https://github.com/Tsoympet/PantheonChain](https://github.com/Tsoympet/PantheonChain)
- Issues: [https://github.com/Tsoympet/PantheonChain/issues](https://github.com/Tsoympet/PantheonChain/issues)

---

**Status**: Production Ready ✅

All 10 development phases complete. Ready for mainnet deployment.
