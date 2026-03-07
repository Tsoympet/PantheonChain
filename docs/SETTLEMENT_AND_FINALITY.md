# Settlement and Finality Model

This document is normative for settlement/finality terminology in PantheonChain MVP.

## 1) TALANTON Finality Model (L1)

- Consensus: SHA-256d Proof-of-Work.
- Finality type: probabilistic finality.
- Settlement strength: highest in the PantheonChain stack.
- Operational interpretation: confirmation depth determines rollback risk tolerance.

TALANTON is the root settlement chain. DRACHMA and OBOLOS do not replace TALANTON as the base settlement layer.

## 2) DRACHMA Finality Model (L2)

- Consensus: PoS validator set with BFT-style quorum finalization.
- Finality type: fast economic finality.
- Safety dependence: validator honesty threshold and key-security assumptions.

DRACHMA finality is materially strengthened when L2 checkpoints are accepted on TALANTON.

## 3) OBOLOS Finality Model (L3)

- Consensus: PoS validator set with BFT-style quorum finalization.
- Finality type: fast economic finality for EVM execution outcomes.
- Safety dependence: validator honesty threshold and operational integrity.

OBOLOS assurances strengthen after sequential checkpointing to DRACHMA and then TALANTON.

## 4) Bridge and Relayer Trust Assumptions

In MVP:

- Trusted/assumed:
  - honest-majority validator behavior on DRACHMA and OBOLOS
  - relayer liveness for publishing commitments
- Verified:
  - commitment payload structure
  - source-chain identity
  - monotonic finalized heights
  - quorum signatures against declared validator sets
- Not yet trustless:
  - validity-proof-based bridging
  - fraud-proof-based trust minimization

## 5) Failure Scenarios

### A. Malicious OBOLOS validator supermajority

Potential impact:
- unsafe L3 state can be economically finalized at L3
- unsafe state may be checkpointed to DRACHMA before detection

### B. Malicious DRACHMA validator supermajority

Potential impact:
- unsafe L2 checkpoints can be produced
- OBOLOS-derived references can be mis-anchored or censored at L2

### C. Relayer censorship or liveness failure

Potential impact:
- checkpoint publication delays
- increased latency to stronger settlement boundaries

### D. TALANTON reorg considerations

Potential impact:
- delayed confidence for recent L2/L3 anchors
- requires conservative confirmation thresholds for high-value settlements

## 6) Security Hierarchy

Settlement assurance ordering is:

`OBOLOS < DRACHMA < TALANTON`

This hierarchy is mandatory for protocol documentation and operator communications.
