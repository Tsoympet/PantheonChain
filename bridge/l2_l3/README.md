# Bridge — L2 ↔ L3 (DRACHMA ↔ OBOLOS)

This module implements the canonical lock-mint / burn-unlock bridge between
DRACHMA (L2) and OBOLOS (L3).

## Token Flow

### DRC → wDRC (Lock-Mint)

```
DRACHMA (L2)                     OBOLOS (L3)
────────────                     ───────────
User calls lock()                Relayer submits proof
  DRC locked in                → Merkle proof verified
  bridge contract                wDRC minted to recipient
  BridgeTransferIntent           (nonce checked, replay-blocked)
  emitted on-chain
```

### wDRC → DRC (Burn-Unlock)

```
OBOLOS (L3)                      DRACHMA (L2)
───────────                      ────────────
User calls burn()                Relayer submits proof
  wDRC burned                  → Merkle proof verified
  BridgeTransferIntent           DRC unlocked to recipient
  emitted on-chain               (nonce checked, replay-blocked)
```

## Security Properties

- **Merkle proof verification** required for every mint/unlock
- **Nonce tracking** prevents replay attacks
- **Checkpoint verification**: L3 state roots are anchored on L2 before
  bridge unlocks are processed on L2
- Relayers are **untrusted** — all security comes from on-chain verification

## Source Files

- Bridge header: [`../cross_chain_message.h`](../cross_chain_message.h)
- L2 bridge implementation: [`../../layer2-drachma/bridges/`](../../layer2-drachma/bridges/)
- L3 relayer: [`../../relayers/relayer-l3/`](../../relayers/relayer-l3/)
