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
4. Finality signatures represent >=2/3 of contributing DRACHMA miner hash power.

## DRACHMA (L2 / PoW)

All DRACHMA commitments anchored to TALANTON must include a state commitment that reflects
the latest finalized OBOLOS commitment hash (`upstream_commitment_hash`).

- Epoch-based proposer rotation.
- Deterministic miner selection weighted by hash power contribution.
- Block finality requires signatures from >=2/3 contributing hash power.
- Miner set is derived from on-chain PoW contribution records.
- **No staking, no slashing.** Miners are penalised by orphaned blocks.

### `TX_L3_COMMIT` Validation Rules

`TX_L3_COMMIT` must satisfy:

1. `source_chain == OBOLOS`
2. `finalized_height` is strictly monotonic against last accepted L3 commitment.
3. Payload fields are correctly encoded.
4. Finality signatures represent >=2/3 of contributing OBOLOS miner hash power.

## OBOLOS (L3 / PoW + EVM)

- Epoch-based PoW with hash-power quorum finality (>=2/3 signatures).
- Full EVM-style execution environment.
- OBOLOS token is used for gas and mining rewards.
- Finalized OBOLOS checkpoints are exported to DRACHMA as commitment payloads.
- **No staking, no slashing.**

`TX_L3_COMMIT` payloads are produced by OBOLOS miners and finalized once signatures
from >=2/3 contributing hash power are collected.

## Governance Voting

All three layers use **balance-based governance voting**:

- Voting power = token balance at snapshot block.
- No staking required; any token holder can vote.
- Snapshots are taken when a proposal enters the ACTIVE state to prevent
  last-block attacks.
- Anti-whale guard (quadratic voting or hard cap) limits plutocratic dominance.
