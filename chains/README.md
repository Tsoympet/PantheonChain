# Chains — PantheonChain Three-Layer Architecture

PantheonChain consists of three independent but interconnected blockchains.
Each token has its own sovereign chain.

## Chain Overview

| Chain | Layer | Token | Consensus | Chain ID |
|-------|-------|-------|-----------|---------|
| [TALANTON](talanton/) | L1 — Root | TLT | Proof-of-Work (SHA256d) | 1001 |
| [DRACHMA](drachma/) | L2 — Economic | DRC | Proof-of-Stake / BFT | 1002 |
| [OBOLOS](obolos/) | L3 — Execution | OBL | Proof-of-Stake / BFT | 1003 |

## Security Hierarchy

```
OBOLOS (L3)  ──checkpoints──▶  DRACHMA (L2)  ──checkpoints──▶  TALANTON (L1)
                                                                  (root of trust)
```

TALANTON does **not** depend on DRACHMA or OBOLOS. Final settlement always
occurs on TALANTON.

## Native Asset Rule

Each token exists **natively only on its own chain**. Other chains may only
hold wrapped representations:

- **TLT** is native on TALANTON → **wTLT** exists on DRACHMA
- **DRC** is native on DRACHMA → **wDRC** exists on OBOLOS
- **OBL** is native on OBOLOS → no wrapped OBL on other chains

**Never mint native tokens outside their origin chain.**

## Replay Protection

Each chain has a unique `chain_id` included in every transaction signature:

| Chain | Chain ID |
|-------|---------|
| TALANTON | 1001 |
| DRACHMA  | 1002 |
| OBOLOS   | 1003 |

Transactions signed for one chain cannot be replayed on another.

## Implementation

Source code for each chain is in:
- `layer1-talanton/` — TALANTON implementation
- `layer2-drachma/` — DRACHMA implementation
- `layer3-obolos/` — OBOLOS implementation

Genesis files:
- `genesis_talanton.json`
- `genesis_drachma.json`
- `genesis_obolos.json`
