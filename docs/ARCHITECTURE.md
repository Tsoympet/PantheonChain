# PantheonChain Architecture (Reference)

This document mirrors the canonical architecture in `docs/architecture.md`.

## Layered Model

PantheonChain uses a strict three-layer stack:

- **TALANTON (L1 / PoW)**: settlement and security anchor.
- **DRACHMA (L2 / PoS+BFT)**: payments and liquidity.
- **OBOLOS (L3 / PoS+BFT)**: EVM execution and gas.

Canonical anchoring path:

`OBOLOS -> DRACHMA -> TALANTON`

## Commitments

- `TX_L3_COMMIT` is produced by OBOLOS and validated on DRACHMA.
- `TX_L2_COMMIT` is produced by DRACHMA and validated on TALANTON.

Both commitment types require:

- monotonic finalized heights,
- deterministic payload encoding,
- >=2/3 active-stake signatures for source-layer finality.

## Codebase Modules

- `src/common`: shared networking, cryptography, serialization, storage, mempool, metrics, commitments.
- `src/talanton`: PoW anchor logic and `TX_L2_COMMIT` validation.
- `src/drachma`: PoS payments layer, staking/slashing, and `TX_L3_COMMIT` validation.
- `src/obolos`: PoS execution layer, EVM integration, gas accounting.
- `relayers`: commitment relay binaries for `l3->l2` and `l2->l1`.

## Interfaces

- Node mode: `pantheon-node --layer=l1|l2|l3`
- CLI:
  - `pantheon-cli stake deposit --layer=l2`
  - `pantheon-cli deploy-contract --layer=l3`
  - `pantheon-cli submit-commitment --layer=l2|l3`

For full operator-level detail, see:

- `docs/architecture.md`
- `docs/consensus.md`
- `docs/threat-model.md`
- `docs/migration.md`
