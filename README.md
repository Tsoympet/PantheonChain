<p align="center">
  <img width="512" height="512" alt="ParthenonChain Logo" src="https://github.com/user-attachments/assets/82446b94-5708-40ec-9155-2c1a52fa8aef" />
</p>

<h1 align="center">ParthenonChain</h1>

<p align="center">
  <strong>A Multi-Asset Proof-of-Work Blockchain with Smart Contracts</strong>
</p>

<p align="center">
  <a href="#-download">Download</a> •
  <a href="#quick-start">Quick Start</a> •
  <a href="#key-features">Features</a> •
  <a href="#installation">Installation</a> •
  <a href="#documentation">Documentation</a> •
  <a href="#development">Development</a> •
  <a href="#contributing">Contributing</a>
</p>

<p align="center">
  <a href="https://github.com/Tsoympet/PantheonChain/releases">
    <img src="https://img.shields.io/github/v/release/Tsoympet/PantheonChain?style=flat-square" alt="Release">
  </a>
  <a href="https://github.com/Tsoympet/PantheonChain/blob/main/LICENSE">
    <img src="https://img.shields.io/badge/license-MIT-blue.svg?style=flat-square" alt="License">
  </a>
  <a href="https://github.com/Tsoympet/PantheonChain/actions">
    <img src="https://img.shields.io/github/actions/workflow/status/Tsoympet/PantheonChain/build.yml?style=flat-square" alt="Build Status">
  </a>
</p>

---

## 📥 Download

**Ready to use ParthenonChain?** Download the installer for your platform:

<table align="center">
<tr>
<td align="center">
<h3>🪟 Windows</h3>
<a href="https://github.com/Tsoympet/PantheonChain/releases/latest">
<b>Download for Windows</b><br/>
<code>parthenon-1.0.0-windows-x64-setup.exe</code>
</a>
<br/><sub>Windows 10/11 (64-bit)</sub>
</td>
<td align="center">
<h3>🍎 macOS</h3>
<a href="https://github.com/Tsoympet/PantheonChain/releases/latest">
<b>Download for macOS</b><br/>
<code>parthenon-1.0.0-macos.dmg</code>
</a>
<br/><sub>macOS 10.15+ (Intel & Apple Silicon)</sub>
</td>
<td align="center">
<h3>🐧 Linux</h3>
<a href="https://github.com/Tsoympet/PantheonChain/releases/latest">
<b>Download for Linux</b><br/>
<code>.deb</code> or <code>.rpm</code>
</a>
<br/><sub>Ubuntu, Debian, Fedora, RHEL</sub>
</td>
</tr>
</table>

<p align="center">
  <strong>📖 New to ParthenonChain?</strong><br/>
  Read the <a href="QUICK_START.md"><b>Quick Start Guide</b></a> or <a href="DOWNLOAD.md"><b>Download Instructions</b></a>
</p>

---

## Overview

ParthenonChain is a production-grade Layer-1 blockchain implementing **SHA-256d Proof-of-Work** consensus with a **multi-asset UTXO ledger**, **Schnorr signatures**, and **EVM-compatible smart contracts**. Built for security, determinism, and extensibility, ParthenonChain combines proven cryptographic foundations with modern blockchain features.

### What Makes ParthenonChain Different?

- 🏛️ **Three Native Tokens**: TALANTON (store of value), DRACHMA (medium of exchange), and OBOLOS (smart contract gas)
- 🔐 **Schnorr Signatures**: BIP-340 implementation for privacy, efficiency, and signature aggregation
- ⚡ **EVM Compatibility**: Run Solidity smart contracts with OBOLOS token for gas
- 🎯 **Production-Ready**: Complete implementation with no placeholders, comprehensive testing
- 📊 **Layered Architecture**: Clear separation between consensus-critical (Layer 1) and optional (Layer 2) components

## Key Features

### Core Blockchain
- **Proof-of-Work Consensus**: SHA-256d mining algorithm compatible with Bitcoin infrastructure
- **Multi-Asset UTXO Model**: Native support for three tokens with distinct economic models
- **Schnorr Signatures**: Advanced cryptography enabling signature aggregation and enhanced privacy
- **Deterministic Execution**: All consensus operations produce identical results across nodes

### Smart Contracts
- **EVM-Compatible Engine**: Run Ethereum smart contracts on ParthenonChain
- **Gas Economics**: EIP-1559 style fee market with base fee and priority tips
- **State Management**: Merkle Patricia Trie for efficient state verification
- **Security**: Gas limits and deterministic execution prevent malicious contracts

### Additional Features
- **DRM Settlement Primitives**: Multi-signature escrow and time-locked transfers for digital rights
- **Layer 2 Protocols**: Payment channels, HTLC bridges, and indexers
- **HD Wallets**: BIP-32/BIP-44 hierarchical deterministic key derivation
- **P2P Network**: Robust gossip protocol with DoS protection and eclipse attack mitigation

## Quick Start

### 🚀 Easiest: Download Pre-built Installer (Recommended for Users)

**For end users**: Download and run the installer for your operating system:

👉 **[Download ParthenonChain](https://github.com/Tsoympet/PantheonChain/releases/latest)** - Get the installer (.exe, .dmg, .deb, or .rpm)

See the **[Quick Start Guide](QUICK_START.md)** for step-by-step installation instructions.

After installation:
```bash
# Start the node
parthenond

# Check node status
parthenon-cli getinfo

# Create wallet address
parthenon-cli getnewaddress
```

### 🐳 For Developers: Running with Docker

The fastest way to run a ParthenonChain node for development:

```bash
# Clone the repository
git clone https://github.com/Tsoympet/PantheonChain.git
cd PantheonChain

# Start the node with Docker Compose
docker-compose up -d

# View logs
docker-compose logs -f parthenond

# Check node status
docker exec -it parthenon-node parthenon-cli getinfo
```

### Using Pre-built Binaries (Alternative)

If you prefer manual installation without the installer:

Download the latest release binaries from the [Releases](https://github.com/Tsoympet/PantheonChain/releases) page.

```bash
# Linux/macOS
./parthenond -daemon

# Check node status
./parthenon-cli getinfo

# View help
./parthenond --help
```

## Installation

### System Requirements

- **OS**: Linux (Ubuntu 20.04+, Debian 10+), macOS 10.15+, Windows 10+
- **CPU**: 2+ cores (4+ recommended)
- **RAM**: 4 GB minimum (8 GB+ recommended)
- **Storage**: 100 GB+ SSD (for full node)
- **Network**: Broadband internet connection

### Building from Source

#### Prerequisites

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake git \
  libssl-dev libevent-dev libboost-all-dev \
  pkg-config autoconf automake libtool
```

**macOS:**
```bash
brew install cmake boost openssl libevent autoconf automake libtool
```

**Windows:**
- Install [Visual Studio 2019+](https://visualstudio.microsoft.com/) with C++ support
- Install [CMake](https://cmake.org/download/)

#### Build Steps

```bash
# Clone the repository
git clone --recursive https://github.com/Tsoympet/PantheonChain.git
cd PantheonChain

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# Run tests
ctest --output-on-failure

# Install (optional)
sudo cmake --install .
```

### Docker Deployment

#### Using Docker Compose

```bash
# Start node
docker-compose up -d

# View logs
docker-compose logs -f parthenond

# Stop node
docker-compose down

# Restart node
docker-compose restart

# Access CLI
docker exec -it parthenon-node parthenon-cli [command]
```

#### Manual Docker Commands

```bash
# Build image
docker build -t parthenon-node .

# Run node
docker run -d \
  -p 8333:8333 \
  -p 8332:8332 \
  -v parthenon-data:/home/parthenon/.parthenon \
  --name parthenon-node \
  parthenon-node

# Check logs
docker logs -f parthenon-node

# Execute CLI commands
docker exec parthenon-node parthenon-cli getinfo
```

## Usage

### Running a Full Node

```bash
# Start the daemon
parthenond -daemon

# Check sync status
parthenon-cli getblockchaininfo

# Get wallet balance (all assets)
parthenon-cli getbalance

# Get specific asset balance
parthenon-cli getbalance "TALANTON"
parthenon-cli getbalance "DRACHMA"
parthenon-cli getbalance "OBOLOS"
```

### Creating Transactions

```bash
# Generate new address
parthenon-cli getnewaddress

# Send TALANTON
parthenon-cli sendtoaddress <address> <amount> TALANTON

# Send DRACHMA
parthenon-cli sendtoaddress <address> <amount> DRACHMA

# Send OBOLOS
parthenon-cli sendtoaddress <address> <amount> OBOLOS
```

### Mining

```bash
# Start mining
parthenon-cli setgenerate true <num_threads>

# Check mining status
parthenon-cli getmininginfo

# Stop mining
parthenon-cli setgenerate false
```

### Smart Contracts

```bash
# Deploy contract
parthenon-cli deploycontract <bytecode> <gas_limit>

# Call contract function
parthenon-cli callcontract <contract_address> <data> <gas_limit>

# Get contract state
parthenon-cli getcontractinfo <contract_address>
```

## Architecture

ParthenonChain employs a strict layered architecture:

```
┌─────────────────────────────────────────────────────┐
│                    CLIENTS                          │
│  (parthenond, CLI, Desktop GUI, Mobile Wallet)      │
└─────────────────────────────────────────────────────┘
                       │
┌─────────────────────────────────────────────────────┐
│                  LAYER 2 (Extensions)               │
│  Payment Channels, Bridges, Indexers, APIs          │
└─────────────────────────────────────────────────────┘
                       │
┌─────────────────────────────────────────────────────┐
│            LAYER 1 (Consensus Critical)             │
│  ┌───────────────────────────────────────────────┐  │
│  │         Core Consensus Engine                 │  │
│  │  PoW, UTXO Set, Mempool, Block Validation     │  │
│  └───────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────┐  │
│  │       Smart Contracts (OBOLOS)                │  │
│  │  EVM Engine, Gas Metering, State Root         │  │
│  └───────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────┐  │
│  │         DRM Settlement                        │  │
│  │  Multi-sig, Time locks, Destination tags      │  │
│  └───────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

**Layer 1** contains consensus-critical code requiring network-wide coordination for changes.  
**Layer 2** contains optional extensions that evolve independently without consensus changes.

For detailed architecture documentation, see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Documentation

### 🚀 Getting Started (For Users)

- 📥 **[Download Guide](DOWNLOAD.md)** - Download and install ParthenonChain
- ⚡ **[Quick Start](QUICK_START.md)** - Get started in minutes
- 🆘 **[Troubleshooting](TROUBLESHOOTING.md)** - Common issues and solutions
- 📱 **[Client Status](CLIENT_STATUS.md)** - All clients and their readiness
- 📸 **[Screenshots Guide](docs/SCREENSHOTS.md)** - Client UI screenshots and capture guidelines

### 📚 Technical Documentation (For Developers)

- 📖 **[Whitepaper](WHITEPAPER.md)** - Technical specification and design rationale
- 🏗️ **[Architecture](docs/ARCHITECTURE.md)** - System design and component overview
- 🔐 **[Security Model](docs/SECURITY_MODEL.md)** - Security properties and threat model
- 🚀 **[Installation Guide](docs/INSTALLATION.md)** - Detailed setup instructions
- 📚 **[API Reference](docs/NETWORKING_RPC.md)** - RPC commands and network protocol
- 🔧 **[Advanced Features](docs/ADVANCED_FEATURES.md)** - DRM, smart contracts, and more
- 📝 **[Changelog](CHANGELOG.md)** - Version history and release notes

## Development

### Setting Up Development Environment

```bash
# Clone with submodules
git clone --recursive https://github.com/Tsoympet/PantheonChain.git
cd PantheonChain

# Build in debug mode
mkdir build-debug && cd build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)
```

### Running Tests

```bash
# Run all tests
cd build
ctest --output-on-failure

# Run specific test suite
./layer1/core/crypto/test_crypto
./layer1/core/consensus/test_consensus
./layer1/core/evm/test_evm

# Run with verbose output
ctest -V
```

### Code Style

ParthenonChain follows modern C++17 standards with strict compiler warnings:

- **Format**: ClangFormat configuration in `.clang-format`
- **Linting**: Enable all warnings (`-Wall -Wextra -Wpedantic -Werror`)
- **Testing**: Comprehensive unit and integration tests required
- **Documentation**: Doxygen-style comments for public APIs

### Project Structure

```
PantheonChain/
├── layer1/               # Consensus-critical Layer 1
│   ├── core/            # Core blockchain (crypto, consensus, UTXO)
│   │   ├── crypto/      # SHA-256, Schnorr signatures
│   │   ├── consensus/   # PoW, difficulty, validation
│   │   ├── utxo/        # Multi-asset UTXO management
│   │   └── evm/         # Smart contract execution
│   ├── network/         # P2P networking
│   └── rpc/             # RPC server
├── layer2/              # Optional Layer 2 extensions
│   ├── channels/        # Payment channels
│   ├── bridges/         # Cross-chain bridges
│   └── indexer/         # Transaction indexing
├── clients/             # Client applications
│   ├── cli/             # Command-line interface
│   ├── desktop/         # Desktop GUI (Qt)
│   └── mobile/          # Mobile wallets
├── docs/                # Documentation
├── tests/               # Integration tests
└── third_party/         # External dependencies
```

## Contributing

We welcome contributions from the community! ParthenonChain is open-source and thrives on collaborative development.

### How to Contribute

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** your changes (`git commit -m 'Add amazing feature'`)
4. **Push** to the branch (`git push origin feature/amazing-feature`)
5. **Open** a Pull Request

### Contribution Guidelines

- Follow the existing code style and conventions
- Add tests for new functionality
- Update documentation as needed
- Ensure all tests pass before submitting PR
- Write clear, descriptive commit messages
- One feature per pull request

### Areas for Contribution

- 🐛 **Bug Fixes**: Report and fix bugs
- ✨ **Features**: Implement new features from the roadmap
- 📚 **Documentation**: Improve docs, tutorials, and examples
- 🧪 **Testing**: Add test coverage and test scenarios
- 🌍 **Translations**: Translate documentation and UI
- 🎨 **UI/UX**: Improve wallet interfaces and user experience

For major changes, please open an issue first to discuss proposed changes.

## Community

Stay connected with the ParthenonChain community:

- 💬 **Discussions**: [GitHub Discussions](https://github.com/Tsoympet/PantheonChain/discussions)
- 🐛 **Issues**: [GitHub Issues](https://github.com/Tsoympet/PantheonChain/issues)
- 📧 **Email**: [Contact maintainers](mailto:dev@parthenonchain.org)
- 📖 **Wiki**: [Project Wiki](https://github.com/Tsoympet/PantheonChain/wiki)

### Getting Help

- Check the [documentation](docs/) first
- Search [existing issues](https://github.com/Tsoympet/PantheonChain/issues)
- Ask in [GitHub Discussions](https://github.com/Tsoympet/PantheonChain/discussions)
- Join community forums and chat channels

## Roadmap

### Current Release (v1.0.0)
✅ Core blockchain with multi-asset UTXO  
✅ Schnorr signatures (BIP-340)  
✅ EVM-compatible smart contracts  
✅ P2P networking and mining  
✅ RPC server and CLI wallet  
✅ Hardware wallet integration (Ledger, Trezor, KeepKey)  
✅ GraphQL and WebSocket APIs  
✅ SPV light client support  
✅ Lightning-style payment channels  
✅ Cross-chain atomic swaps (HTLC)  
✅ Desktop wallet application (Qt-based)  
✅ Mobile wallet application (React Native)  
✅ Multi-signature wallet support  
✅ Enhanced privacy features (Ring signatures, Stealth addresses)  
✅ Advanced smart contract debugging tools  
✅ Blockchain explorer web interface  
✅ Decentralized exchange (DEX) with AMM  
✅ Hardware wallet firmware verification  
✅ zkSNARK privacy enhancements  
✅ Cross-shard transactions  
✅ Mobile SDK libraries (iOS/Android)  
✅ IDE plugins for smart contract development  
✅ Layer 2 rollup scaling solutions (Optimistic + ZK-Rollups)  
✅ Formal verification framework  
✅ Cross-chain bridges to major blockchains (Bitcoin, Ethereum, etc.)  
✅ Quantum-resistant cryptography (Dilithium, Kyber, SPHINCS+)  
✅ Decentralized identity (DID) system (W3C compliant)  
✅ Privacy-preserving smart contracts (Private ERC-20, auctions, voting)  
✅ Advanced ZK-STARK integration (transparent zero-knowledge proofs)  
✅ Homomorphic encryption support (BFV and CKKS schemes)  
✅ Interplanetary File System (IPFS) integration  
✅ Machine learning on-chain verification  

**🎉 Feature Complete - All planned features implemented! 🎉**

### Future Possibilities
- Second-generation quantum cryptography
- Advanced consensus mechanisms
- Next-generation virtual machine architectures

See [CHANGELOG.md](CHANGELOG.md) for detailed version history and [docs/RELEASES.md](docs/RELEASES.md) for the full roadmap.

## Releases

### Latest Release

**Version 1.0.0** is available for download!

👉 **[Download ParthenonChain](https://github.com/Tsoympet/PantheonChain/releases/latest)** - Get installers for Windows, macOS, and Linux

### All Releases

View all releases: [GitHub Releases](https://github.com/Tsoympet/PantheonChain/releases)

Each release includes:
- 📦 **Windows installer** (.exe) - NSIS setup for Windows 10/11
- 📦 **macOS disk image** (.dmg) - Universal binary for Intel & Apple Silicon
- 📦 **Linux packages** (.deb, .rpm) - For Debian/Ubuntu and RHEL/Fedora
- 🔐 **Checksums** - SHA-256/SHA-512 verification
- ✍️ **GPG signatures** - Authenticity verification (when available)

### Installation

See our comprehensive guides:
- 🚀 [Quick Start Guide](QUICK_START.md) - Get started in minutes
- 📥 [Download Instructions](DOWNLOAD.md) - Detailed installation steps
- 📖 [Installation Guide](docs/INSTALLATION.md) - Complete documentation

### Building from Source

Prefer to build yourself? See the [Building from Source](#building-from-source) section above or [installers/README.md](installers/README.md) for creating installers.

## Security

Security is our top priority. ParthenonChain implements defense-in-depth security with deterministic consensus, cryptographic rigor, and comprehensive validation.

### Reporting Vulnerabilities

**Please DO NOT file public issues for security vulnerabilities.**

Report security issues privately:
- 📧 Email: security@parthenonchain.org
- 🔒 GitHub Security Advisories: [Report a vulnerability](https://github.com/Tsoympet/PantheonChain/security/advisories/new)

See [SECURITY.md](SECURITY.md) for our full security policy and responsible disclosure process.

## License

ParthenonChain is released under the [MIT License](LICENSE).

```
Copyright (c) 2026 ParthenonChain Foundation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction...
```

See [LICENSE](LICENSE) file for full license text.

## Acknowledgments

ParthenonChain builds upon the work of:

- **Bitcoin Core** - Proof-of-Work consensus and UTXO model
- **Ethereum** - Smart contract execution and EVM design
- **libsecp256k1** - Cryptographic primitives for Schnorr signatures
- **LevelDB** - Blockchain storage and state management

Special thanks to all [contributors](https://github.com/Tsoympet/PantheonChain/graphs/contributors) who have helped build ParthenonChain.

---

<p align="center">
  <strong>Built with ❤️ by the ParthenonChain community</strong>
</p>

<p align="center">
  <a href="https://github.com/Tsoympet/PantheonChain">GitHub</a> •
  <a href="WHITEPAPER.md">Whitepaper</a> •
  <a href="docs/">Documentation</a> •
  <a href="https://github.com/Tsoympet/PantheonChain/releases">Releases</a>
</p>
