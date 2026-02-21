# Layer 2 Protocols

Layer 2 components are **non-consensus** extensions that enhance ParthenonChain without requiring changes to the consensus protocol.

## Overview

Layer 2 provides:
- **Scalability**: Off-chain transaction processing
- **Interoperability**: Cross-chain bridges
- **Usability**: Enhanced query and notification APIs
- **Flexibility**: Can be upgraded independently

## Architecture

```
┌─────────────────────────────────────────────┐
│          Layer 2 Applications               │
├─────────────────────────────────────────────┤
│  Payment    │  Bridges  │ Indexers │  APIs  │
│  Channels   │ HTLC/SPV  │          │GraphQL │
└─────────────────────────────────────────────┘
       │              │            │        │
       └──────────────┴────────────┴────────┘
                      │
       ┌──────────────┴──────────────┐
       │      Layer 1 Consensus       │
       └──────────────────────────────┘
```

## Payment Channels

**Location**: `layer2/payment_channels/`

### Overview

Bidirectional payment channels enable instant, low-cost transactions between two parties without broadcasting every payment to the blockchain.

### Channel Lifecycle

1. **Opening**:
   - Both parties fund a multi-sig address on Layer 1
   - Initial channel state is recorded

2. **Updates**:
   - Parties exchange signed state updates off-chain
   - Each update increments sequence number
   - Updates can flow in both directions

3. **Closing**:
   - Cooperative: Both parties sign final state, broadcast to Layer 1
   - Unilateral: One party broadcasts latest known state
   - Dispute: Counterparty has time window to submit newer state

### Channel State

```cpp
struct ChannelState {
    uint8_t channel_id[32];
    uint8_t party_a[32];      // Public key
    uint8_t party_b[32];      // Public key
    uint64_t balance_a;       // Party A's balance
    uint64_t balance_b;       // Party B's balance
    uint64_t sequence;        // State version number
    uint8_t sig_a[64];        // Party A's signature (Schnorr)
    uint8_t sig_b[64];        // Party B's signature (Schnorr)
};
```

### Implementation

**Files**:
- `ChannelState.h` / `ChannelState.cpp`

**Features**:
- Balance updates
- Signature verification
- State serialization
- Dispute resolution helpers

### Use Cases

- Micropayments for content streaming
- Gaming transactions
- Machine-to-machine payments
- High-frequency trading

## Bridges

**Location**: `layer2/bridges/`

### HTLC (Hash Time Locked Contracts)

#### Overview

HTLCs enable atomic swaps between ParthenonChain and other blockchains.

#### HTLC Structure

```cpp
struct HTLC {
    uint8_t hash_lock[32];    // SHA-256 hash of secret
    uint32_t time_lock;       // Expiry timestamp
    uint8_t recipient[32];    // Recipient if secret revealed
    uint8_t refund_addr[32];  // Refund address after timeout
    uint64_t amount;
};
```

#### Atomic Swap Flow

1. **Alice (ParthenonChain) ↔ Bob (Other Chain)**:
   - Alice generates secret `s`, computes `h = SHA256(s)`
   - Alice creates HTLC on ParthenonChain: "Pay Bob if he reveals `s` before time T"
   - Bob creates HTLC on other chain: "Pay Alice if she reveals `s` before time T-1"
   - Bob reveals `s` on other chain to claim Alice's payment
   - Alice sees `s`, uses it to claim Bob's payment on ParthenonChain

2. **Timeout**:
   - If swap doesn't complete, both parties can reclaim funds after timeout

#### Implementation

**Files**: `htlc/HTLC.h`, `htlc/HTLC.cpp`

**Functions**:
- `create_htlc()` - Create new HTLC
- `claim_htlc()` - Claim with secret
- `refund_htlc()` - Reclaim after timeout

### SPV (Simplified Payment Verification)

#### Overview

SPV enables light clients to verify ParthenonChain transactions without downloading the full blockchain.

#### Merkle Proof

```cpp
struct MerkleProof {
    uint8_t tx_hash[32];
    uint8_t block_hash[32];
    uint32_t tx_index;
    std::vector<uint8_t[32]> merkle_path;
};
```

#### Verification

```cpp
bool VerifySPV(const MerkleProof& proof) {
    uint8_t hash[32];
    memcpy(hash, proof.tx_hash, 32);
    
    for (const auto& sibling : proof.merkle_path) {
        sha256d(hash, sibling, hash);  // Combine and hash
    }
    
    return memcmp(hash, proof.block_header.merkle_root, 32) == 0;
}
```

#### Implementation

**Files**: `spv/SPVProof.h`, `spv/SPVProof.cpp`

**Use Cases**:
- Mobile wallets
- Cross-chain bridges
- Light clients

## Indexers

**Location**: `layer2/indexers/`

### Transaction Indexer

#### Overview

Indexes all transactions by address, enabling fast balance and history queries.

#### Database Schema

```sql
-- Transactions
CREATE TABLE transactions (
    txid BLOB PRIMARY KEY,
    block_hash BLOB,
    block_height INTEGER,
    timestamp INTEGER,
    version INTEGER
);

-- Outputs
CREATE TABLE outputs (
    txid BLOB,
    output_index INTEGER,
    address TEXT,
    amount INTEGER,
    asset INTEGER,
    spent BOOLEAN,
    PRIMARY KEY (txid, output_index)
);

CREATE INDEX idx_address ON outputs(address);
```

#### Features

- Address balance queries
- Transaction history
- UTXO set queries
- Rich list generation

### Contract Event Indexer

#### Overview

Indexes smart contract events for fast querying.

#### Event Structure

```cpp
struct ContractEvent {
    uint8_t contract_address[20];
    std::vector<uint8_t[32]> topics;
    std::vector<uint8_t> data;
    uint32_t block_height;
    uint8_t tx_hash[32];
    uint32_t log_index;
};
```

#### Database Schema

```sql
CREATE TABLE contract_events (
    block_height INTEGER,
    tx_hash BLOB,
    log_index INTEGER,
    contract_address BLOB,
    topic0 BLOB,
    topic1 BLOB,
    topic2 BLOB,
    topic3 BLOB,
    data BLOB,
    PRIMARY KEY (tx_hash, log_index)
);

CREATE INDEX idx_contract ON contract_events(contract_address);
CREATE INDEX idx_topic0 ON contract_events(topic0);
```

#### Use Cases

- DApp event tracking
- Token transfer monitoring
- Contract state history

## APIs

**Location**: `layer2/apis/`

### GraphQL API

#### Overview

GraphQL endpoint for flexible blockchain queries.

#### Schema

```graphql
type Query {
  block(hash: String, height: Int): Block
  transaction(hash: String!): Transaction
  address(addr: String!): Address
  contract(addr: String!): Contract
}

type Block {
  hash: String!
  height: Int!
  timestamp: Int!
  transactions: [Transaction!]!
  miner: String!
}

type Transaction {
  hash: String!
  block: Block
  inputs: [Input!]!
  outputs: [Output!]!
  fee: Int!
}

type Address {
  address: String!
  balance: Balance!
  transactions: [Transaction!]!
}

type Balance {
  talanton: Int!
  drachma: Int!
  obolos: Int!
}
```

#### Example Queries

```graphql
# Get block with transactions
{
  block(height: 12345) {
    hash
    timestamp
    transactions {
      hash
      fee
    }
  }
}

# Get address balance and history
{
  address(addr: "ptn1q...") {
    balance {
      talanton
      drachma
      obolos
    }
    transactions(limit: 10) {
      hash
      block {
        height
      }
    }
  }
}
```

### WebSocket API

#### Overview

Real-time notifications for blockchain events.

#### Subscriptions

```javascript
// Subscribe to new blocks
ws.send(JSON.stringify({
  action: "subscribe",
  topic: "blocks"
}));

// Subscribe to address transactions
ws.send(JSON.stringify({
  action: "subscribe",
  topic: "address",
  address: "ptn1q..."
}));

// Subscribe to contract events
ws.send(JSON.stringify({
  action: "subscribe",
  topic: "contract_events",
  contract: "0x..."
}));
```

#### Events

```javascript
// New block
{
  type: "block",
  data: {
    hash: "...",
    height: 12345,
    timestamp: 1234567890
  }
}

// New transaction
{
  type: "transaction",
  data: {
    hash: "...",
    from: "...",
    to: "...",
    amount: 1000000
  }
}
```

## Layer 2 Development

### Adding New Layer 2 Features

1. **Create module in `layer2/`**
2. **Implement functionality**
3. **Add tests** (not consensus-critical, but still important)
4. **Document API**
5. **Deploy** (can be done independently)

### Best Practices

- **Never modify Layer 1** from Layer 2 code
- **Use Layer 1 as source of truth**
- **Handle chain reorgs** gracefully
- **Validate all Layer 1 data**
- **Design for scalability**

## Deployment

Layer 2 components can run:
- **Integrated**: Within `parthenond` (indexers)
- **Standalone**: Separate services (APIs, bridges)
- **Third-party**: Community-run infrastructure

## Future Layer 2 Extensions

Planned enhancements:
- Lightning-style payment channel networks
- Decentralized exchange (DEX) integration
- Oracle services
- Advanced analytics APIs
- Cross-chain bridges to Ethereum, Bitcoin

---

**See Also**:
- [architecture.md](architecture.md)
