# Bridge — L1 ↔ L2 (TALANTON ↔ DRACHMA)

This module implements the canonical lock-mint / burn-unlock bridge between
TALANTON (L1) and DRACHMA (L2).

## Token Flow

### TLT → wTLT (Lock-Mint)

```
TALANTON (L1)                    DRACHMA (L2)
──────────────                   ────────────
User calls lock()                Relayer submits proof
  TLT locked in                → Merkle proof verified
  bridge contract                wTLT minted to recipient
  BridgeTransferIntent           (nonce checked, replay-blocked)
  emitted on-chain
```

### wTLT → TLT (Burn-Unlock)

```
DRACHMA (L2)                     TALANTON (L1)
────────────                     ──────────────
User calls burn()                Relayer submits proof
  wTLT burned                  → Merkle proof verified
  BridgeTransferIntent           L1 bridge confirms ≥6 confirmations
  emitted on-chain               TLT unlocked to recipient
```

## Security Properties

- **Merkle proof verification** required for every mint/unlock
- **Nonce tracking** prevents replay attacks
- **Minimum confirmations**: `bridge_unlock_min_l1_depth = 6` (L1 blocks)
- **Fraud proof detection**: challengers can submit fraud proofs within the
  challenge window
- Relayers are **untrusted** — they only transport messages; all security
  comes from on-chain verification

## Source Files

- Bridge header: [`../cross_chain_message.h`](../cross_chain_message.h)
- L1 bridge implementation: [`../../layer1-talanton/tx/l1_commitment_validator.cpp`](../../layer1-talanton/tx/l1_commitment_validator.cpp)
- L2 bridge implementation: [`../../layer2-drachma/bridges/`](../../layer2-drachma/bridges/)
- L2 relayer: [`../../relayers/relayer-l2/`](../../relayers/relayer-l2/)
