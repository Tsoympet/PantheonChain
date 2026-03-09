# Consensus — PantheonChain Consensus Modules

This directory provides canonical consensus module references for each layer.

## Architecture

PantheonChain uses **asymmetric consensus** across its three layers:

| Chain | Layer | Consensus | Module |
|-------|-------|-----------|--------|
| TALANTON | L1 | Proof-of-Work (SHA256d) | `pow/` |
| DRACHMA  | L2 | Proof-of-Stake / BFT   | `pos/` |
| OBOLOS   | L3 | Proof-of-Stake / BFT   | `pos/` |

## Security Hierarchy

TALANTON does **not** depend on DRACHMA or OBOLOS for its security.
PoW and PoS validation are strictly separated.

```
OBOLOS (L3, PoS/BFT)
    │ checkpoints to
    ▼
DRACHMA (L2, PoS/BFT)
    │ checkpoints to
    ▼
TALANTON (L1, PoW-SHA256d)   ← root of trust
```

## Subdirectories

- `pow/` — Proof-of-Work consensus (TALANTON only)
- `pos/` — Proof-of-Stake / BFT consensus (DRACHMA and OBOLOS)

## Source Implementations

- PoW: [`../layer1-talanton/core/consensus/`](../layer1-talanton/core/consensus/)
- PoS L2: [`../layer2-drachma/consensus/`](../layer2-drachma/consensus/)
- PoS L3: [`../layer3-obolos/consensus/`](../layer3-obolos/consensus/)
