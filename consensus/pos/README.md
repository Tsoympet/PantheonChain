# Proof-of-Stake / BFT Consensus (DRACHMA L2 + OBOLOS L3)

Both DRACHMA and OBOLOS use a **Proof-of-Stake with BFT finality** consensus
mechanism. Validators are selected proportionally to their staked tokens.

## Validator Selection

Proposer selection uses **stake-weighted round-robin**. The proposer for block
`h` is:

```
proposer = validators[sum_of_stakes_weighted_round_robin(h)]
```

## Finality

Blocks achieve **BFT finality** when ≥ ⌈2/3⌉ of the active validator set
(by stake weight) have signed the block. Once finalised, a block cannot be
reverted without slashing the supermajority of validators.

## Slashing Conditions

| Condition | Slash Amount |
|-----------|-------------|
| Double signing | 5% of stake |
| Equivocation | 10% of stake |

Slashed validators are removed from the active set for a configurable unbonding
period.

## DRACHMA (L2) Parameters

| Parameter | Value |
|-----------|-------|
| Minimum stake | 100,000 DRC |
| Epoch length | 120 blocks |
| Commitment interval | 10 blocks |
| Double-sign slash | 5% |
| Equivocation slash | 10% |

## OBOLOS (L3) Parameters

| Parameter | Value |
|-----------|-------|
| Minimum stake | 50,000 OBL |
| Epoch length | 60 blocks |
| Commitment interval | 5 blocks |
| Double-sign slash | 5% |
| Equivocation slash | 10% |

## Checkpoint Production

### OBOLOS → DRACHMA

Every 5 OBOLOS blocks, validators produce a checkpoint containing:
- `block_hash`
- `state_root`
- `height`
- `timestamp`
- validator signatures (≥ ⌈2/3⌉ by stake)

### DRACHMA → TALANTON

Every 10 DRACHMA blocks, validators produce a checkpoint containing the
same fields, and also reference the latest OBOLOS checkpoint hash.

## Source Implementations

- L2 consensus: [`../../layer2-drachma/consensus/pos_consensus.h`](../../layer2-drachma/consensus/pos_consensus.h)
- L3 consensus: [`../../layer3-obolos/consensus/pos_consensus.h`](../../layer3-obolos/consensus/pos_consensus.h)
