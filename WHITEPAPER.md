# PantheonChain Whitepaper (MVP Architecture)

## Abstract

PantheonChain is a hybrid layered blockchain network that separates settlement, payments, and execution into distinct chains with explicit trust boundaries.

- **TALANTON (L1):** Proof-of-Work base settlement and anchoring chain.
- **DRACHMA (L2):** Proof-of-Stake + BFT payments layer with fast economic finality.
- **OBOLOS (L3):** Proof-of-Stake + BFT EVM execution and governance layer.

The canonical checkpoint route is **OBOLOS -> DRACHMA -> TALANTON**. This document defines the consensus model, commitment formats, token roles, trust assumptions, and MVP limitations without claiming cryptographic properties that are not implemented.

---

## 1. Design Goals

1. Keep TALANTON as the only consensus-critical PoW settlement root.
2. Provide high-throughput payments on DRACHMA with PoS/BFT finality.
3. Provide EVM execution and governance on OBOLOS with PoS/BFT finality.
4. Preserve explicit settlement hierarchy through chained checkpoints.
5. Avoid ambiguity that all three tokens co-secure one shared consensus engine.

---

## 2. Layer Model

### 2.1 TALANTON — Layer 1 (PoW)

- Consensus: SHA-256d PoW.
- Finality: probabilistic (heaviest valid chain).
- Responsibilities:
  - root settlement
  - immutable anchor for higher-layer checkpoints
- Native token: **TALANTON**
  - PoW mining rewards
  - L1 transaction fees / settlement utility

### 2.2 DRACHMA — Layer 2 (PoS + BFT payments)

- Consensus: PoS validator set with BFT quorum finalization.
- Finality: fast economic finality under validator honesty assumptions.
- Responsibilities:
  - payments and liquidity routing
  - checkpointing to TALANTON
  - carrying latest accepted OBOLOS commitment reference
- Native token: **DRACHMA**
  - validator staking collateral
  - L2 fee utility

### 2.3 OBOLOS — Layer 3 (PoS + BFT + EVM)

- Consensus: PoS validator set with BFT quorum finalization.
- Finality: fast economic finality under validator honesty assumptions.
- Responsibilities:
  - EVM execution
  - governance and DeFi state transitions
  - checkpoint publishing to DRACHMA
- Native token: **OBOLOS**
  - gas
  - staking collateral
  - governance utility

### 2.4 Canonical checkpoint route

`OBOLOS -> DRACHMA -> TALANTON`

DRACHMA commitments include the latest finalized OBOLOS checkpoint hash to maintain transitive anchoring.

---

## 3. Commitment Transactions

### 3.1 `TX_L2_COMMIT` (recorded on TALANTON)

`source_chain = DRACHMA`

Required fields:

- `epoch`
- `finalized_height`
- `finalized_block_hash`
- `state_root`
- `validator_set_hash`
- `validator_signatures`
- `upstream_commitment_hash` (latest accepted OBOLOS commitment reference)

Validation requirements on TALANTON:

1. monotonic finalized height
2. valid payload encoding
3. quorum signatures >= 2/3 active DRACHMA validator stake

### 3.2 `TX_L3_COMMIT` (recorded on DRACHMA)

`source_chain = OBOLOS`

Required fields mirror `TX_L2_COMMIT` excluding upstream recursion:

- `epoch`
- `finalized_height`
- `finalized_block_hash`
- `state_root`
- `validator_set_hash`
- `validator_signatures`

Validation requirements on DRACHMA:

1. monotonic finalized height
2. valid payload encoding
3. quorum signatures >= 2/3 active OBOLOS validator stake

---

## 4. Consensus and Finality Semantics

### 4.1 TALANTON finality

TALANTON settlement is PoW-based and probabilistic. Higher confirmation depth increases rollback cost and therefore settlement confidence.

### 4.2 DRACHMA and OBOLOS finality

DRACHMA and OBOLOS provide fast BFT-style economic finality under validator quorum assumptions. This finality is operationally strong but not equivalent to TALANTON settlement finality.

### 4.3 Settlement hierarchy

Security ordering for settlement assurance:

`OBOLOS < DRACHMA < TALANTON`

Upper-layer finality is strengthened when checkpoints are accepted by downstream layers and ultimately anchored on TALANTON.

---

## 5. Bridging and Asset Flow (MVP)

### 5.1 L1 <-> L2

- Deposit: lock on TALANTON -> mint representation on DRACHMA
- Withdraw: burn on DRACHMA -> unlock on TALANTON

### 5.2 L2 <-> L3

- Deposit: lock on DRACHMA -> mint representation on OBOLOS
- Withdraw: burn on OBOLOS -> unlock on DRACHMA

### 5.3 MVP bridge properties

- Withdrawals are optimistic and subject to a trust window.
- No zk validity proofs are assumed.
- No fraud proof system is assumed.
- Safety depends on validator-honesty and relayer-liveness assumptions.

---

## 6. Token Economics (Role Separation)

PantheonChain preserves three native assets with separate utility domains:

- **TALANTON (TALN):** PoW mining rewards, L1 settlement fees, base-layer security token.
- **DRACHMA (DRM):** L2 validator staking, payment-layer fee utility, L2 checkpoint economy.
- **OBOLOS (OBL):** L3 validator staking, gas, governance, execution-layer economy.

Supply schedule and network-specific issuance parameters are defined in genesis/config artifacts.

---

## 7. Security and Trust Assumptions (MVP)

1. TALANTON PoW remains economically secure against majority hash attacks.
2. DRACHMA safety/liveness depends on validator honesty threshold assumptions.
3. OBOLOS safety/liveness depends on validator honesty threshold assumptions.
4. Relayers are required for checkpoint publication liveness.
5. Checkpoint and bridge flows are not trustless in MVP.

Failure implications:

- OBOLOS validator supermajority compromise can corrupt L3 economic finality before DRACHMA/TALANTON settlement.
- DRACHMA validator supermajority compromise can corrupt L2 finality and downstream trust in OBOLOS-derived anchors.
- Relayer censorship/liveness failures delay settlement progression.
- TALANTON reorg events can delay certainty for recent upper-layer settlements.

---

## 8. Conclusion

PantheonChain is a layered hybrid architecture with explicit consensus separation: TALANTON secures base settlement via PoW, while DRACHMA and OBOLOS deliver higher throughput and faster economic finality through PoS/BFT validator quorums. The model intentionally keeps TALANTON as the root settlement chain and avoids claims of trustless bridging beyond implemented MVP guarantees.
