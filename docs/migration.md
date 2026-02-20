# Migration Guide

## Goal

Migrate from a single-chain, multi-token model to layered operation:

- TALANTON = L1 settlement/security.
- DRACHMA = L2 payments/liquidity.
- OBOLOS = L3 EVM execution.

## Operator Migration Steps

1. Deploy TALANTON nodes as PoW-only settlement anchors.
2. Deploy DRACHMA validator nodes with PoS staking enabled.
3. Deploy OBOLOS validator/execution nodes.
4. Start relayers:
   - `pantheon-relayer-l3` for OBOLOS -> DRACHMA.
   - `pantheon-relayer-l2` for DRACHMA -> TALANTON.
5. Switch client workflows to layer-aware RPC/CLI endpoints.

## Config and Genesis Inputs

Use chain-specific genesis files:

- `genesis_talanton.json`
- `genesis_drachma.json`
- `genesis_obolos.json`

Each genesis includes inflation, epoch length, minimum stake, slashing ratios, and commitment interval parameters.

## Monetary Unit-of-Account Migration Note

Ancient Greek denomination constants are now canonical and protocol-defined:

- `1 DRACHMA = 6 OBOLOS`
- `1 TALANTON = 6000 DRACHMA = 36000 OBOLOS`

Precision remains fixed at 8 decimals for each token (talantonion, drachmion, obolion).

Backward compatibility: raw base units in the ledger remain unchanged (`10^8` base unit scale), so no historical balance rewrite is required.

## Backward Compatibility

Legacy directories remain available for historical compatibility, but new consensus and commitment logic should target `src/common`, `src/talanton`, `src/drachma`, and `src/obolos`.
