# Consensus Model

## TALANTON (L1)

- Proof-of-Work with SHA-256d.
- No staking.
- Validates `TX_L2_COMMIT` with:
  - monotonic finalized height,
  - payload encoding checks,
  - >=2/3 active stake finality signatures.

## DRACHMA (L2)

- Epoch-based PoS with BFT finality.
- Deterministic proposer selection weighted by stake.
- Slashing primitive for double-signing.
- Validates `TX_L3_COMMIT` using monotonicity, encoding, and quorum.

## OBOLOS (L3)

- Epoch-based PoS with BFT finality.
- EVM-style execution with gas accounting.
- Uses OBOLOS as gas and stake token.
