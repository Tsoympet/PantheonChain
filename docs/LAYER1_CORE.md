# Layer 1 Core Implementation

This document provides detailed technical information about ParthenonChain's Layer 1 consensus implementation.

## Overview

Layer 1 is the **consensus-critical** component of ParthenonChain. All nodes must execute Layer 1 code identically to maintain network consensus.

## Cryptographic Primitives

### SHA-256

**Implementation**: `layer1/core/crypto/sha256.cpp`

**Standard**: FIPS 180-4 compliant

**Functions**:
```cpp
void sha256(const uint8_t* data, size_t len, uint8_t* hash);
void sha256d(const uint8_t* data, size_t len, uint8_t* hash);
void tagged_sha256(const char* tag, const uint8_t* msg, size_t len, uint8_t* hash);
```

**Test Vectors**: Verified against NIST and Bitcoin Core test vectors

### Schnorr Signatures

**Implementation**: `layer1/core/crypto/schnorr.cpp`

**Standard**: BIP-340

**Curve**: secp256k1

**Key Format**:
- Private key: 32 bytes
- Public key: 32 bytes (x-only)
- Signature: 64 bytes (r || s)

**Functions**:
```cpp
bool schnorr_sign(const uint8_t* msg32, const uint8_t* seckey32, 
                  const uint8_t* aux_rand32, uint8_t* sig64);
bool schnorr_verify(const uint8_t* msg32, const uint8_t* pubkey32, 
                    const uint8_t* sig64);
```

## Block Structure

### Block Header

```cpp
struct BlockHeader {
    uint32_t version;           // Block version
    uint8_t prev_block[32];     // Previous block hash (SHA-256d)
    uint8_t merkle_root[32];    // Merkle root of transactions
    uint32_t timestamp;         // Block timestamp
    uint32_t bits;              // Difficulty target
    uint32_t nonce;             // Mining nonce
};
```

**Size**: 80 bytes (Bitcoin-compatible)

### Block

```cpp
struct Block {
    BlockHeader header;
    std::vector<Transaction> transactions;
};
```

## Transaction Format

### Transaction Structure

```cpp
struct Transaction {
    uint32_t version;
    std::vector<TxInput> inputs;
    std::vector<TxOutput> outputs;
    uint32_t locktime;
};
```

### Transaction Input

```cpp
struct TxInput {
    uint8_t prev_tx[32];        // Previous transaction hash
    uint32_t prev_index;        // Output index
    std::vector<uint8_t> script_sig;  // Unlocking script
    uint32_t sequence;
};
```

### Transaction Output

```cpp
struct TxOutput {
    uint64_t amount;            // Amount in satoshis
    AssetType asset;            // TALANTON, DRACHMA, or OBOLOS
    std::vector<uint8_t> script_pubkey;  // Locking script
};
```

### Asset Types

```cpp
enum class AssetType : uint8_t {
    TALANTON = 0,  // 21M supply
    DRACHMA = 1,   // 41M supply
    OBOLOS = 2     // 61M supply
};
```

## Consensus Rules

### Proof-of-Work

**Algorithm**: SHA-256d (double SHA-256)

**Target Calculation**:
```
target = 2^256 / difficulty
block_hash < target  // Valid block
```

**Difficulty Adjustment**:
- Retarget every 2016 blocks (~2 weeks)
- Target block time: 10 minutes
- Adjustment formula: `new_diff = old_diff * (actual_time / expected_time)`
- Limits: 0.25x to 4x per adjustment

### Block Rewards

**TALANTON** (Asset 0):
```cpp
uint64_t GetTalantonReward(uint32_t height) {
    uint64_t subsidy = 50 * COIN;  // Initial reward
    uint32_t halvings = height / 210000;
    if (halvings >= 64) return 0;
    return subsidy >> halvings;
}
```

**DRACHMA** (Asset 1):
```cpp
uint64_t GetDrachmaReward(uint32_t height) {
    uint64_t subsidy = 97.619 * COIN;  // 41M / 420000 blocks
    uint32_t halvings = height / 210000;
    if (halvings >= 64) return 0;
    return subsidy >> halvings;
}
```

**OBOLOS** (Asset 2):
```cpp
uint64_t GetObolosReward(uint32_t height) {
    uint64_t subsidy = 145.238 * COIN;  // 61M / 420000 blocks
    uint32_t halvings = height / 210000;
    if (halvings >= 64) return 0;
    return subsidy >> halvings;
}
```

**Total Block Reward**: Sum of all three assets

### Issuance Schedule

| Blocks      | TALANTON/block | DRACHMA/block | OBOLOS/block |
|-------------|----------------|---------------|--------------|
| 0-209,999   | 50.0           | 97.619        | 145.238      |
| 210,000-419,999 | 25.0       | 48.810        | 72.619       |
| 420,000-629,999 | 12.5       | 24.405        | 36.310       |
| ...         | ...            | ...           | ...          |

**Total Supply**:
- TALANTON: 21,000,000
- DRACHMA: 41,000,000
- OBOLOS: 61,000,000

## UTXO Model

### UTXO Set

**Storage**: In-memory with disk persistence

**Structure**:
```cpp
struct UTXO {
    TxOutput output;
    uint32_t height;      // Block height of confirmation
    bool is_coinbase;     // Coinbase output (100 block maturity)
};

std::unordered_map<OutPoint, UTXO> utxo_set;
```

### OutPoint

```cpp
struct OutPoint {
    uint8_t txid[32];
    uint32_t index;
};
```

### UTXO Updates

**On Block Connect**:
1. Remove spent UTXOs
2. Add new UTXOs from block transactions
3. Update state root

**On Block Disconnect** (reorg):
1. Remove UTXOs from disconnected block
2. Restore spent UTXOs
3. Recompute state root

## Block Validation

### Validation Steps

1. **Header Validation**:
   - Check PoW (hash < target)
   - Verify timestamp (not too far in future)
   - Check difficulty matches expected

2. **Transaction Validation**:
   - Check transaction format
   - Verify no duplicate transactions
   - Validate coinbase transaction
   - Check transaction signatures

3. **UTXO Validation**:
   - Verify all inputs exist
   - Check no double spends
   - Validate amounts (no overflow)
   - Ensure sufficient funds

4. **State Transition**:
   - Apply UTXO updates
   - Compute new state root
   - Update chain tip

### Coinbase Validation

```cpp
bool ValidateCoinbase(const Transaction& tx, uint32_t height) {
    // Must have exactly one input (coinbase marker)
    if (tx.inputs.size() != 1) return false;
    
    // Must have outputs for each asset
    // Total output value must not exceed block reward
    uint64_t tal_out = 0, drm_out = 0, obl_out = 0;
    for (const auto& out : tx.outputs) {
        if (out.asset == TALANTON) tal_out += out.amount;
        else if (out.asset == DRACHMA) drm_out += out.amount;
        else if (out.asset == OBOLOS) obl_out += out.amount;
    }
    
    return tal_out <= GetTalantonReward(height) &&
           drm_out <= GetDrachmaReward(height) &&
           obl_out <= GetObolosReward(height);
}
```

## Script System

### Script Operations

**Stack-based execution**:
- OP_DUP, OP_HASH160, OP_EQUALVERIFY, OP_CHECKSIG
- Multi-sig support
- Time locks (OP_CHECKLOCKTIMEVERIFY)

### Standard Script Types

**Pay-to-PubKey (P2PK)**:
```
scriptPubKey: <pubkey> OP_CHECKSIG
scriptSig: <signature>
```

**Pay-to-PubKey-Hash (P2PKH)**:
```
scriptPubKey: OP_DUP OP_HASH160 <pubkeyhash> OP_EQUALVERIFY OP_CHECKSIG
scriptSig: <signature> <pubkey>
```

**Pay-to-Script-Hash (P2SH)**:
```
scriptPubKey: OP_HASH160 <scripthash> OP_EQUAL
scriptSig: <signature> <redeem_script>
```

## Mempool

### Transaction Pool

**Data Structure**:
```cpp
struct MempoolEntry {
    Transaction tx;
    uint64_t fee;
    uint64_t time;
    uint64_t height;
};

std::map<uint256, MempoolEntry> mempool;  // Ordered by fee rate
```

### Fee Calculation

```cpp
uint64_t CalculateFee(const Transaction& tx) {
    uint64_t input_value = 0;
    for (const auto& input : tx.inputs) {
        input_value += GetUTXOValue(input);
    }
    
    uint64_t output_value = 0;
    for (const auto& output : tx.outputs) {
        output_value += output.amount;
    }
    
    return input_value - output_value;
}
```

### Mempool Policies

- **Minimum Fee**: 1 satoshi per byte
- **Maximum Size**: 300 MB
- **Eviction**: Lowest fee rate first
- **Replacement**: RBF (Replace-By-Fee) supported

## P2P Network

### Network Messages

```cpp
enum MessageType {
    VERSION,      // Node version handshake
    VERACK,       // Version acknowledgment
    GETADDR,      // Request peer addresses
    ADDR,         // Peer addresses
    GETBLOCKS,    // Request block inventory
    INV,          // Inventory (blocks/transactions)
    GETDATA,      // Request full block/transaction
    BLOCK,        // Block data
    TX,           // Transaction data
    PING,         // Keepalive
    PONG          // Keepalive response
};
```

### Peer Discovery

1. DNS seeds
2. Hardcoded seed nodes
3. Peer address exchange (ADDR messages)

### Block Propagation

**Compact Blocks**: Send block headers + transaction hashes, request missing transactions

**Orphan Handling**: Request missing parent blocks

## State Root

### Merkle Tree

**UTXO Commitment**:
```cpp
uint256 ComputeUTXOMerkleRoot(const UTXOSet& utxo_set) {
    std::vector<uint256> hashes;
    for (const auto& [outpoint, utxo] : utxo_set) {
        hashes.push_back(Hash(outpoint, utxo));
    }
    return ComputeMerkleRoot(hashes);
}
```

## Reorganization

### Reorg Handling

```cpp
bool Reorganize(const Block& new_best_block) {
    // 1. Find fork point
    Block* fork_point = FindForkPoint(new_best_block);
    
    // 2. Disconnect blocks on old chain
    while (chain_tip != fork_point) {
        DisconnectBlock(chain_tip);
        chain_tip = chain_tip->prev;
    }
    
    // 3. Connect blocks on new chain
    for (Block* block : new_chain_blocks) {
        if (!ConnectBlock(block)) {
            // Reorg failed, restore old chain
            return false;
        }
    }
    
    return true;
}
```

## Checkpoints

Hardcoded block hashes to prevent deep reorgs:

```cpp
const std::map<uint32_t, uint256> CHECKPOINTS = {
    {0, GENESIS_BLOCK_HASH},
    {10000, BLOCK_10000_HASH},
    {50000, BLOCK_50000_HASH},
};
```

## Network Parameters

```cpp
struct NetworkParams {
    uint32_t magic;              // Network magic bytes
    uint16_t default_port;       // P2P port
    std::vector<std::string> dns_seeds;
    uint256 genesis_hash;
    uint32_t pow_limit;          // Maximum difficulty target
    uint32_t pow_target_spacing; // 600 seconds (10 minutes)
    uint32_t pow_target_timespan; // 2016 blocks
};
```

**Mainnet**:
- Magic: 0xF9BEB4D9
- Port: 8333
- Genesis: [See GENESIS.md](GENESIS.md)

**Testnet**:
- Magic: 0x0B110907
- Port: 18333

## Performance Optimizations

- **UTXO Caching**: LRU cache for frequently accessed UTXOs
- **Script Caching**: Cache script validation results
- **Parallel Validation**: Validate transactions in parallel
- **Batch Signature Verification**: Verify multiple signatures together

## Constants

```cpp
const uint64_t COIN = 100000000;  // 1 coin = 10^8 satoshis
const uint32_t MAX_BLOCK_SIZE = 4000000;  // 4 MB
const uint32_t COINBASE_MATURITY = 100;  // Blocks before coinbase can be spent
const uint32_t MAX_MONEY = 21000000 * COIN;  // Maximum TALANTON
```

---

**See Also**:
- [architecture.md](architecture.md)
- [SECURITY_MODEL.md](SECURITY_MODEL.md)
