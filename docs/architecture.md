# Architecture

PantheonChain is a layered modular blockchain stack.

```mermaid
flowchart TD
    O[OBOLOS L3\nPoS+BFT\nEVM + gas] --> D[DRACHMA L2\nPoS+BFT\nPayments + liquidity]
    D --> T[TALANTON L1\nPoW SHA-256d\nSettlement + security anchor]
```

## Responsibilities

- **TALANTON (L1):** PoW consensus, settlement, immutable root of trust.
- **DRACHMA (L2):** high-throughput payments, staking/slashing, L3 commitment aggregation.
- **OBOLOS (L3):** contract execution and gas accounting.

## Anchoring path

`OBOLOS -> DRACHMA -> TALANTON`

## Source layout

- `src/common`: shared commitment/model utilities.
- `src/talanton`: L1 anchoring validation.
- `src/drachma`: PoS payments-layer helpers.
- `src/obolos`: execution-layer helpers.
- `src/relayers`: relayer binaries.
