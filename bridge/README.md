# Bridge — Cross-Chain Protocol

This directory contains the canonical bridge implementation for PantheonChain's
three-layer architecture.

## Structure

```
bridge/
├── cross_chain_message.h   — Canonical message standard (all cross-chain messages)
├── l1_l2/                  — TALANTON (L1) ↔ DRACHMA (L2) bridge
│   ├── README.md
│   ├── l1_l2_bridge.h
│   └── l1_l2_bridge.cpp
└── l2_l3/                  — DRACHMA (L2) ↔ OBOLOS (L3) bridge
    ├── README.md
    ├── l2_l3_bridge.h
    └── l2_l3_bridge.cpp
```

## Cross-Chain Message Standard

All messages use the `CrossChainMessage` struct defined in `cross_chain_message.h`.

Required fields:
| Field | Description |
|-------|-------------|
| `origin_chain_id` | Source chain (TALANTON=1001, DRACHMA=1002, OBOLOS=1003) |
| `destination_chain_id` | Target chain |
| `message_nonce` | Monotonic counter per sender (replay protection) |
| `block_height` | Block height on origin chain |
| `timestamp` | Unix timestamp of origin block |
| `payload_hash` | SHA256d of serialised payload |
| `state_root` | MPT state root at `block_height` |
| `proof` | Merkle inclusion proof nodes |
| `validator_signatures` | ≥ ⌈2/3⌉ validators by stake weight |

## Bridge Protocol

### Lock-Mint (native → wrapped)

| Route | Native Token | Wrapped Token |
|-------|-------------|---------------|
| L1 → L2 | TLT on TALANTON | wTLT on DRACHMA |
| L2 → L3 | DRC on DRACHMA | wDRC on OBOLOS |

### Burn-Unlock (wrapped → native)

| Route | Wrapped Token | Native Token |
|-------|--------------|--------------|
| L2 → L1 | Burn wTLT on DRACHMA | Unlock TLT on TALANTON |
| L3 → L2 | Burn wDRC on OBOLOS | Unlock DRC on DRACHMA |

## Security

Bridges are **trust-minimised** (not trustless). Security properties:

1. **Merkle proof verification** — every mint/unlock requires a valid proof
2. **Nonce tracking** — replay protection enforced on-chain
3. **Minimum confirmations** — L1 unlock requires ≥6 L1 block confirmations
4. **Validator signatures** — ≥ ⌈2/3⌉ of active validator set required
5. **Fraud proofs** — challengers can dispute invalid state transitions
6. **Relayers are untrusted** — relayers only transport messages; all
   security enforcement is on-chain

## Native Token Sovereignty

Per the PantheonChain architecture rules:

- TLT exists **natively only on TALANTON**. wTLT on DRACHMA is a wrapped representation.
- DRC exists **natively only on DRACHMA**. wDRC on OBOLOS is a wrapped representation.
- OBL exists **natively only on OBOLOS**. There is no wrapped OBL on other chains.

**Never mint native tokens outside their origin chain.**
