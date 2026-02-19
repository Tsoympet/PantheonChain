# Migration

## What changes for existing users

- Node operators now choose an explicit layer role: `--layer=l1|l2|l3`.
- Configs are profile-based under `configs/`.
- Devnet orchestration uses three node instances plus relayers.
- RPC and CLI now include layered commitment/staking/execution command families.

## Backward compatibility

Legacy folders (`layer1/`, `layer2/`) remain for compatibility; new work should target layered `src/` modules.
# Migration Guide

## From monolithic chain to layered stack

1. Keep TALANTON PoW consensus as L1 settlement anchor.
2. Move payment-heavy flows to DRACHMA (L2 PoS).
3. Move contract execution to OBOLOS (L3 PoS + EVM).
4. Route commitments through canonical path `L3 -> L2 -> L1`.
5. Update operators to use `pantheon-node --layer=<l1|l2|l3>`.

## Config and genesis

Use chain-specific genesis files:

- `genesis_talanton.json`
- `genesis_drachma.json`
- `genesis_obolos.json`
