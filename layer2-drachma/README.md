# Layer 2 Extensions

Non-consensus extensions and protocols for ParthenonChain.

## Overview

Layer 2 components enhance ParthenonChain without requiring changes to the consensus protocol. They provide scalability, interoperability, and enhanced user experience.

## Components

### Payment Channels (`payment_channels/`)

Bidirectional payment channels for instant, low-cost off-chain transactions.

**Features**:
- Channel opening and funding
- Off-chain state updates
- Cooperative and unilateral closing
- Dispute resolution

**Use Cases**:
- Micropayments
- Content streaming payments
- Gaming transactions
- High-frequency trading

**See**: [ChannelState.h](payment_channels/ChannelState.h)

### Bridges (`bridges/`)

Cross-chain interoperability protocols.

#### HTLC (`bridges/htlc/`)

Hash Time Locked Contracts for atomic swaps.

**Features**:
- Atomic cross-chain swaps
- Hash locks for secret revelation
- Time locks for refunds
- Trustless exchange

#### SPV (`bridges/spv/`)

Simplified Payment Verification for light clients.

**Features**:
- Merkle proof verification
- Light client support
- Minimal storage requirements
- Trust-minimized verification

### Indexers (`indexers/`)

Database indexing for enhanced query capabilities.

#### Transaction Indexer (`indexers/tx_indexer/`)

Indexes transactions by address for fast balance and history queries.

**Features**:
- Address balance tracking
- Transaction history
- UTXO queries
- Rich list generation

#### Contract Indexer (`indexers/contract_indexer/`)

Indexes smart contract events and state changes.

**Features**:
- Event log indexing
- Contract state tracking
- Efficient querying
- DApp support

### APIs (`apis/`)

External data access interfaces.

#### GraphQL API (`apis/graphql/`)

Flexible GraphQL endpoint for blockchain queries.

**Capabilities**:
- Block queries
- Transaction queries
- Address queries
- Contract queries
- Custom queries

#### WebSocket API (`apis/websocket/`)

Real-time event notifications.

**Subscriptions**:
- New blocks
- New transactions
- Address updates
- Contract events

## Building Layer 2

```bash
# Build all Layer 2 components
mkdir build && cd build
cmake -DBUILD_LAYER2=ON ..
make layer2
```

## Running Layer 2 Services

### Indexers

```bash
# Start transaction indexer
./build/layer2/indexers/tx_indexer

# Start contract indexer
./build/layer2/indexers/contract_indexer
```

### APIs

```bash
# Start GraphQL API
./build/layer2/apis/graphql_server --port 8080

# Start WebSocket API
./build/layer2/apis/websocket_server --port 8081
```

## Architecture

Layer 2 components are designed to:
- **Not affect consensus**: Failures don't impact blockchain validity
- **Scale independently**: Can be deployed on separate infrastructure
- **Upgrade freely**: No coordination needed for updates
- **Use Layer 1 as source of truth**: Always validate against blockchain

## Development

### Adding a New Layer 2 Module

1. Create directory under `layer2/`
2. Implement functionality
3. Add CMakeLists.txt
4. Write tests
5. Document API
6. Deploy independently

### Best Practices

- **Never modify Layer 1 state** from Layer 2
- **Handle reorganizations** gracefully
- **Validate all Layer 1 data**
- **Design for horizontal scaling**
- **Provide clear APIs**

## Layer Separation

```
┌─────────────────────────────┐
│        Layer 2              │
│  (Non-consensus)            │
│  - Can fail without         │
│    affecting blockchain     │
│  - Upgradeable              │
│  - Scalable                 │
└─────────────┬───────────────┘
              │ Read-only
┌─────────────▼───────────────┐
│        Layer 1              │
│  (Consensus Critical)       │
│  - Must never fail          │
│  - Requires coordination    │
│  - Deterministic            │
└─────────────────────────────┘
```

## Testing

```bash
# Run Layer 2 tests
ctest -R layer2

# Test specific component
ctest -R payment_channels
ctest -R indexers
```

## Documentation

For detailed Layer 2 protocol documentation, see:
- [LAYER2_PROTOCOLS.md](../docs/LAYER2_PROTOCOLS.md)
- [Architecture Overview](../docs/architecture.md)
- [Phase 8 Completion Report](../docs/PHASE8_COMPLETION.md)

## Examples

### Payment Channel Usage

```cpp
#include "payment_channels/ChannelState.h"

// Create channel
ChannelState channel(party_a_pubkey, party_b_pubkey, initial_balance_a, initial_balance_b);

// Update state
channel.UpdateBalance(new_balance_a, new_balance_b);
channel.SignState(party_a_privkey, party_b_privkey);

// Close channel (broadcast final state to Layer 1)
Transaction close_tx = channel.GenerateCloseTx();
```

### SPV Verification

```cpp
#include "bridges/spv/SPVProof.h"

// Verify transaction inclusion
SPVProof proof = GetProofFromNode(txid);
bool valid = proof.Verify(block_header);
```

## Deployment Options

Layer 2 components can be deployed:

1. **Integrated**: Within `parthenond` for convenience
2. **Standalone**: Separate processes for scalability
3. **Distributed**: Multiple instances for redundancy
4. **Third-party**: Community-run infrastructure

## Future Extensions

Planned Layer 2 enhancements:
- Lightning Network-style payment channel routing
- Decentralized exchange protocols
- Oracle integration
- Advanced analytics
- Cross-chain bridges to major blockchains

---

**For support**: See main [README.md](../README.md) for contact information.
