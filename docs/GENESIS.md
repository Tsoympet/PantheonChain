# Genesis Block

The ParthenonChain genesis block is the foundation of the blockchain.

## Genesis Block Details

### Block Header

```
Version: 1
Previous Block: 0x0000000000000000000000000000000000000000000000000000000000000000
Merkle Root: [Calculated from coinbase transaction]
Timestamp: 1704067200 (2024-01-01 00:00:00 UTC)
Bits: 0x1d00ffff (Difficulty 1)
Nonce: [To be mined]
```

### Block Hash

```
0x[Will be calculated after mining]
```

### Coinbase Transaction

The genesis block contains a single coinbase transaction that creates the initial coin supply for each asset.

#### Transaction Structure

```
Version: 1
Inputs: 1
  - Coinbase input with message
Outputs: 3
  - TALANTON output
  - DRACHMA output
  - OBOLOS output
Locktime: 0
```

#### Coinbase Message

```
"ParthenonChain Genesis - 2024-01-01 - The Times 01/Jan/2024"
```

This message serves as:
1. Proof that the chain started on or after January 1, 2024
2. A timestamp anchor similar to Bitcoin's genesis block
3. A nod to blockchain tradition

#### Genesis Outputs

**TALANTON (Asset 0)**:
- Amount: 50.00000000 TALANTON (5,000,000,000 satoshis)
- Address: [Genesis address - coins unspendable by convention]

**DRACHMA (Asset 1)**:
- Amount: 97.61900000 DRACHMA
- Address: [Genesis address - coins unspendable by convention]

**OBOLOS (Asset 2)**:
- Amount: 145.23800000 OBOLOS
- Address: [Genesis address - coins unspendable by convention]

### Genesis Difficulty

Starting difficulty: `0x1d00ffff`

This is the same starting difficulty as Bitcoin, corresponding to:
- Target: `0x00000000ffff0000000000000000000000000000000000000000000000000000`
- Difficulty: 1

### Mining the Genesis Block

The genesis block must be mined to meet the difficulty target:

```cpp
BlockHeader genesis_header = {
    .version = 1,
    .prev_block = {0},  // All zeros
    .merkle_root = CalculateMerkleRoot(coinbase_tx),
    .timestamp = 1704067200,
    .bits = 0x1d00ffff,
    .nonce = 0  // Start at 0, increment until valid
};

// Mine until hash meets target
while (!ValidatePoW(genesis_header)) {
    genesis_header.nonce++;
}
```

## Genesis State

### UTXO Set

After genesis block, the UTXO set contains exactly 3 outputs (one for each asset).

### State Root

Initial state root is the Merkle root of the genesis UTXO set.

## Network Parameters

### Mainnet

```cpp
GenesisParams mainnet_genesis = {
    .timestamp = 1704067200,
    .bits = 0x1d00ffff,
    .nonce = [calculated],
    .coinbase_message = "ParthenonChain Genesis - 2024-01-01",
    .talanton_output = 50 * COIN,
    .drachma_output = 97.619 * COIN,
    .obolos_output = 145.238 * COIN
};
```

### Testnet

```cpp
GenesisParams testnet_genesis = {
    .timestamp = 1704067200,
    .bits = 0x1d00ffff,  // Same starting difficulty
    .nonce = [different from mainnet],
    .coinbase_message = "ParthenonChain Testnet Genesis - 2024-01-01",
    // Same output amounts
};
```

### Regtest

```cpp
GenesisParams regtest_genesis = {
    .timestamp = 1704067200,
    .bits = 0x207fffff,  // Very low difficulty
    .nonce = 0,
    .coinbase_message = "ParthenonChain Regtest Genesis",
    // Same output amounts
};
```

## Verification

To verify you're on the correct chain, check:

1. **Genesis block hash matches**
2. **Coinbase message is correct**
3. **Initial outputs are correct**
4. **Timestamp is 1704067200**

```cpp
bool VerifyGenesisBlock(const Block& block) {
    if (block.header.prev_block != uint256(0)) return false;
    if (block.header.timestamp != 1704067200) return false;
    if (block.header.bits != 0x1d00ffff) return false;
    if (block.transactions.size() != 1) return false;
    
    const auto& coinbase = block.transactions[0];
    if (coinbase.inputs.size() != 1) return false;
    if (coinbase.outputs.size() != 3) return false;
    
    // Verify coinbase message
    const auto& script = coinbase.inputs[0].script_sig;
    std::string message(script.begin(), script.end());
    if (message.find("ParthenonChain Genesis") == std::string::npos) {
        return false;
    }
    
    // Verify outputs
    uint64_t tal_out = 0, drm_out = 0, obl_out = 0;
    for (const auto& out : coinbase.outputs) {
        if (out.asset == TALANTON) tal_out = out.amount;
        if (out.asset == DRACHMA) drm_out = out.amount;
        if (out.asset == OBOLOS) obl_out = out.amount;
    }
    
    if (tal_out != 50 * COIN) return false;
    if (drm_out != 97.619 * COIN) return false;
    if (obl_out != 145.238 * COIN) return false;
    
    // Verify PoW
    if (!ValidatePoW(block.header)) return false;
    
    return true;
}
```

## Genesis Block Special Rules

1. **Unspendable**: Genesis outputs are conventionally unspendable (similar to Bitcoin)
2. **Not in UTXO set**: Some implementations exclude genesis outputs from the UTXO set
3. **Hardcoded**: Genesis block hash is hardcoded in software
4. **Cannot be reorganized**: Genesis block can never be replaced

## Historical Significance

The genesis block marks the birth of ParthenonChain. Its timestamp and message anchor the chain to a specific point in time, providing:

- **Provable start date**: Cannot have been created before the timestamp
- **Cultural significance**: Links to blockchain tradition
- **Immutability**: First block that all others build upon

## Technical Implementation

Genesis block is defined in `layer1/core/consensus/genesis.cpp`:

```cpp
Block GetGenesisBlock(NetworkType network) {
    Block genesis;
    
    // Build coinbase transaction
    Transaction coinbase;
    coinbase.version = 1;
    coinbase.locktime = 0;
    
    // Input: Coinbase
    TxInput input;
    input.prev_tx = {0};
    input.prev_index = 0xFFFFFFFF;
    const char* msg = "ParthenonChain Genesis - 2024-01-01";
    input.script_sig.assign(msg, msg + strlen(msg));
    coinbase.inputs.push_back(input);
    
    // Outputs: One for each asset
    TxOutput tal_out, drm_out, obl_out;
    tal_out.amount = 50 * COIN;
    tal_out.asset = TALANTON;
    drm_out.amount = 97.619 * COIN;
    drm_out.asset = DRACHMA;
    obl_out.amount = 145.238 * COIN;
    obl_out.asset = OBOLOS;
    
    coinbase.outputs = {tal_out, drm_out, obl_out};
    
    // Build block
    genesis.transactions.push_back(coinbase);
    genesis.header.version = 1;
    genesis.header.prev_block = {0};
    genesis.header.merkle_root = CalculateMerkleRoot(genesis.transactions);
    genesis.header.timestamp = 1704067200;
    genesis.header.bits = 0x1d00ffff;
    genesis.header.nonce = GetGenesisNonce(network);
    
    return genesis;
}
```

## Conclusion

The genesis block is the immutable foundation of ParthenonChain. All subsequent blocks reference it directly or indirectly through the chain of previous block hashes.

---

**See Also**:
- [Consensus Rules](LAYER1_CORE.md#consensus-rules)
- [Issuance Schedule](LAYER1_CORE.md#block-rewards)
- [Network Parameters](LAYER1_CORE.md#network-parameters)
