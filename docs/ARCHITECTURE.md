# ParthenonChain Architecture

## Overview

ParthenonChain is designed as a **layered blockchain system** with strict separation between consensus-critical components (Layer 1) and optional extensions (Layer 2).

## Design Principles

1. **Determinism**: All consensus code produces identical results across all nodes
2. **Layer Separation**: Clear boundaries between Layer 1 (consensus) and Layer 2 (extensions)
3. **Security First**: Cryptographic primitives and validation logic are rigorously tested
4. **Production Readiness**: In progress; refer to the implementation gap audit for remaining work

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         CLIENTS                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │parthenond│  │parthenon │  │ Desktop  │  │  Mobile  │   │
│  │  (node)  │  │   -cli   │  │   GUI    │  │  Wallet  │   │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘   │
└───────┼─────────────┼─────────────┼─────────────┼──────────┘
        │             │             │             │
        └─────────────┴─────────────┴─────────────┘
                       │
┌──────────────────────┴──────────────────────────────────────┐
│                      LAYER 2 (Extensions)                    │
│  ┌─────────────┐  ┌─────────┐  ┌──────────┐  ┌─────────┐  │
│  │  Payment    │  │ Bridges │  │ Indexers │  │  APIs   │  │
│  │  Channels   │  │(HTLC/SPV)│ │          │  │(GraphQL)│  │
│  └─────────────┘  └─────────┘  └──────────┘  └─────────┘  │
└──────────────────────────────────────────────────────────────┘
                       │
┌──────────────────────┴──────────────────────────────────────┐
│                  LAYER 1 (Consensus Critical)                │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              Core Consensus Engine                    │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐           │  │
│  │  │   PoW    │  │ UTXO Set │  │ Mempool  │           │  │
│  │  │ Consensus│  │          │  │          │           │  │
│  │  └──────────┘  └──────────┘  └──────────┘           │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │            Smart Contracts (OBOLOS)                   │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐           │  │
│  │  │   EVM    │  │   Gas    │  │  State   │           │  │
│  │  │ Engine   │  │ Metering │  │  Root    │           │  │
│  │  └──────────┘  └──────────┘  └──────────┘           │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │               DRM Settlement                          │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐           │  │
│  │  │ MultiSig │  │  Escrow  │  │   Tags   │           │  │
│  │  └──────────┘  └──────────┘  └──────────┘           │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │          Cryptographic Primitives                     │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐           │  │
│  │  │ SHA-256d │  │ Schnorr  │  │  Tagged  │           │  │
│  │  │   PoW    │  │   Sigs   │  │   Hash   │           │  │
│  │  └──────────┘  └──────────┘  └──────────┘           │  │
│  └──────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

## Layer 1: Consensus Critical

### Core Components

#### 1. Cryptographic Primitives (`layer1/core/crypto/`)

**Purpose**: Foundation of all cryptographic operations

**Components**:
- `sha256.cpp/h`: SHA-256 and SHA-256d (double hash)
- `schnorr.cpp/h`: BIP-340 Schnorr signatures on secp256k1
- Tagged hashing for domain separation

**Dependencies**: 
- `libsecp256k1` (third_party/secp256k1)

**Characteristics**:
- FIPS 180-4 compliant
- Deterministic
- Constant-time operations (via libsecp256k1)
- Bitcoin-compatible

#### 2. Primitives & Data Structures (`layer1/core/primitives/`)

**Purpose**: Basic blockchain data types

**Components**:
- Block structure (header + transactions)
- Transaction format (multi-asset UTXO)
- Script system
- Merkle trees
- Serialization

**Features**:
- Three native assets (TALANTON, DRACHMA, OBOLOS)
- UTXO-based transaction model
- Script-based locking/unlocking

#### 3. Consensus (`layer1/core/consensus/`)

**Purpose**: Proof-of-Work and issuance rules

**Components**:
- `difficulty.cpp/h`: Difficulty adjustment algorithm
- `issuance.cpp/h`: Asset issuance schedules and block rewards
- `pow.cpp/h`: Proof-of-Work validation

**Rules**:
- SHA-256d mining (Bitcoin-compatible)
- Difficulty retargeting
- Fixed supply schedules:
  - TALANTON: 21M (halving every 210,000 blocks)
  - DRACHMA: 41M
  - OBOLOS: 61M

#### 4. Chainstate (`layer1/core/chainstate/`)

**Purpose**: Blockchain state management

**Components**:
- `chain.cpp/h`: Best chain tracking
- `utxo.cpp/h`: UTXO set database
- `chainstate.cpp/h`: Complete state manager

**Responsibilities**:
- Maintain UTXO set
- Track best chain
- Handle reorganizations
- Compute state roots

#### 5. Validation (`layer1/core/validation/`)

**Purpose**: Block and transaction validation

**Components**:
- Block validation (PoW, transactions, state transitions)
- Transaction validation (signatures, inputs, outputs)
- Script execution
- Consensus rule enforcement

**Validation Steps**:
1. Proof-of-Work check
2. Block structure validation
3. Transaction validation
4. UTXO updates
5. State root computation

#### 6. Mempool (`layer1/core/mempool/`)

**Purpose**: Unconfirmed transaction pool

**Features**:
- Fee-based prioritization
- Transaction dependencies
- Replacement by fee (RBF)
- Eviction policies

#### 7. P2P Networking (`layer1/core/p2p/`)

**Purpose**: Peer-to-peer communication

**Components**:
- Peer discovery
- Block propagation
- Transaction relay
- Network message protocol

#### 8. Mining (`layer1/core/mining/`)

**Purpose**: Block production

**Features**:
- Block template generation
- Transaction selection
- PoW computation
- Block submission

### Smart Contracts (OBOLOS) (`layer1/evm/`)

**Purpose**: EVM-compatible smart contract execution

**Components**:
- `vm.cpp/h`: Virtual machine implementation
- `opcodes.cpp/h`: EVM opcodes
- `state.cpp/h`: Contract state management

**Features**:
- Deterministic execution
- Gas metering (EIP-1559 style)
- Stack-based VM
- State root tracking
- Contract deployment and calls

**Gas Economics**:
- Base fee (burned)
- Priority tip (to miner)
- Dynamic fee adjustment

### DRM Settlement (`layer1/settlement/`)

**Purpose**: Digital rights management primitives

**Components**:
- `multisig.cpp/h`: Multi-signature transactions
- `escrow.cpp/h`: Escrow accounts
- `destination_tag.cpp/h`: Payment routing tags

**Use Cases**:
- Content licensing
- Royalty distribution
- Rights transfers
- Payment channels

## Layer 2: Extensions

Layer 2 components are **non-consensus** and can be modified without network-wide coordination.

### Payment Channels (`layer2/payment_channels/`)

**Purpose**: Off-chain micropayments

**Features**:
- Bidirectional channels
- Channel updates
- Dispute resolution (settled on Layer 1)

### Bridges (`layer2/bridges/`)

**Purpose**: Cross-chain interoperability

**Components**:
- HTLC (Hash Time Locked Contracts)
- SPV (Simplified Payment Verification)

### Indexers (`layer2/indexers/`)

**Purpose**: Enhanced query capabilities

**Components**:
- Transaction indexer
- Contract event indexer
- Address balance tracker

### APIs (`layer2/apis/`)

**Purpose**: External data access

**Components**:
- GraphQL API
- WebSocket API (real-time updates)
- REST endpoints

## Clients

### parthenond (`clients/core-daemon/`)

**Purpose**: Full node daemon

**Capabilities**:
- Full block validation
- UTXO set maintenance
- P2P networking
- Mining
- RPC server

### parthenon-cli (`clients/cli/`)

**Purpose**: Command-line interface

**Commands**:
- Wallet operations
- Transaction creation
- Blockchain queries
- Network management

### Desktop GUI (`clients/desktop/`)

**Purpose**: Graphical wallet

**Technology**: Qt

**Features**:
- Transaction history
- Address management
- Network statistics
- Settings

### Mobile Wallet (`clients/mobile/`)

**Purpose**: Mobile wallet + mining

**Technology**: React Native

**Features**:
- iOS/Android support
- SPV mode
- QR code support
- Share-mining

## Data Flow

### Block Propagation

```
Miner → Block Created
  ↓
Validation (PoW, Transactions)
  ↓
UTXO Set Update
  ↓
State Root Computation
  ↓
Add to Chain
  ↓
Broadcast to Peers
```

### Transaction Flow

```
User → Transaction Created
  ↓
Signature Generation (Schnorr)
  ↓
Broadcast to Network
  ↓
Mempool Validation
  ↓
Included in Block Template
  ↓
Miner Mines Block
  ↓
Transaction Confirmed
```

### Smart Contract Execution

```
User → Contract Call Transaction
  ↓
EVM Execution
  ↓
Gas Metering
  ↓
State Updates
  ↓
State Root Computation
  ↓
Transaction Result
```

## Security Model

See [SECURITY_MODEL.md](SECURITY_MODEL.md) for details.

**Key Points**:
- Layer 1 is deterministic and fully validated
- Layer 2 cannot affect consensus
- Cryptographic operations are constant-time
- All inputs are validated

## Performance Characteristics

**Block Time**: ~10 minutes (Bitcoin-style)
**Block Size**: Configurable (consensus parameter)
**Transaction Throughput**: Layer 1 limited, Layer 2 unbounded
**Finality**: Probabilistic (6 confirmations recommended)

## Build System

**Technology**: CMake

**Structure**:
```
build/
├── layer1/           # Consensus libraries
├── layer2/           # Extension libraries
├── clients/          # Executables
└── tests/            # Test binaries
```

## Testing Strategy

1. **Unit Tests**: Individual component testing
2. **Integration Tests**: Multi-component workflows
3. **Consensus Tests**: Edge cases and attack vectors
4. **Fuzz Tests**: Random input generation
5. **Regression Tests**: Known issues

## Versioning

- **Consensus Protocol**: Version 1
- **Network Protocol**: Version 1
- **API Version**: 1.0

Changes to consensus rules require hard fork coordination.

## Future Extensions

Planned but not yet implemented:

- Schnorr aggregation for reduced signature size
- Taproot-style script trees
- Cross-chain atomic swaps
- Lightning-style payment channels
- Zero-knowledge proofs

---

**For detailed implementation notes**:
- [Layer 1 Core](LAYER1_CORE.md)
- [Layer 2 Protocols](LAYER2_PROTOCOLS.md)
- [Security Model](SECURITY_MODEL.md)
