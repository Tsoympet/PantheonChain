# Consensus Specification

## TALANTON (L1 / PoW)

- Algorithm: SHA-256d Proof-of-Work.
- TALANTON does not run staking.
- Native token utility is restricted to mining rewards and L1 fees.

### `TX_L2_COMMIT` Validation Rules

`TX_L2_COMMIT` must satisfy:

1. `source_chain == DRACHMA`
2. `finalized_height` is strictly monotonic relative to anchored L2 height.
3. Payload fields are correctly encoded.
4. Finality signatures represent >=2/3 of active DRACHMA stake.

## DRACHMA (L2 / PoS + BFT)

- Epoch-based proposer rotation.
- Deterministic proposer selection weighted by active stake.
- Block finality requires signatures from >=2/3 active stake.
- Validator set is derived from staking state.

### Slashing Conditions

- Double-signing.
- Equivocation.

### `TX_L3_COMMIT` Validation Rules

`TX_L3_COMMIT` must satisfy:

1. `source_chain == OBOLOS`
2. `finalized_height` is strictly monotonic against last accepted L3 commitment.
3. Payload fields are correctly encoded.
4. Finality signatures represent >=2/3 of active OBOLOS stake.

## OBOLOS (L3 / PoS + BFT + EVM)

- Epoch-based PoS with BFT finality (>=2/3 signatures).
- Full EVM-style execution environment.
- OBOLOS token is used for gas and staking.
- Finalized OBOLOS checkpoints are exported to DRACHMA as commitment payloads.
