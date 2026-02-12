# ParthenonChain Tokenomics

## Overview

ParthenonChain uses a **multi-asset UTXO model** with three native tokens. Each token has a fixed supply cap and a distinct issuance schedule designed for its economic role.

## Token Summary

| Token | Symbol | Max Supply | Block Reward Schedule | Primary Use |
|-------|--------|------------|----------------------|-------------|
| TALANTON | TAL | 21,000,000 | Bitcoin-like halving | Store of value, mining rewards |
| DRACHMA | DRA | 41,000,000 | Linear decrease | Medium of exchange |
| OBOLOS | OBL | 61,000,000 | Exponential decay | Smart contract gas, DRM fees |

## Issuance Schedule

**TALANTON (TAL):**
- Initial reward: 50 TAL per block
- Halving every 210,000 blocks (~4 years)
- Converges to 21M maximum supply

**DRACHMA (DRA):**
- Initial reward: 100 DRA per block
- Linear decrease over 410,000 blocks
- Final supply: 41M DRA

**OBOLOS (OBL):**
- Initial reward: 200 OBL per block
- Exponential decay with half-life of 100,000 blocks
- Asymptotically approaches 61M OBL

## Economic Roles

- **Talanton (TAL)** secures the network through mining rewards and long-term value preservation.
- **Drachma (DRA)** provides a transactional medium optimized for everyday payments.
- **Obolos (OBL)** powers smart contract execution and DRM settlement, with EIP-1559 style fee dynamics and base-fee burning.

## Further Reading

For the full economic model and formal specifications, see the [ParthenonChain Whitepaper](../WHITEPAPER.md).
