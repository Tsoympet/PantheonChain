# PantheonChain Whitepaper

**Version 2.0 (Layered Refactor Edition)**  
**Date: January 2026**

---

## Abstract

PantheonChain is a modular blockchain stack that separates security, payments, and execution into three anchored layers:

- **TALANTON (L1)** for Proof-of-Work settlement and root security.
- **DRACHMA (L2)** for Proof-of-Stake payments and liquidity.
- **OBOLOS (L3)** for Proof-of-Stake EVM execution.

The system uses rollup-style commitment anchoring with BFT finality signatures on PoS layers. The canonical commitment path is **OBOLOS -> DRACHMA -> TALANTON**. This whitepaper defines the architecture, consensus model, commitment transaction formats, bridging flows, and MVP trust assumptions.

---

## 1. Design Goals

1. **Security minimization at L1**: Keep the settlement layer simple, immutable, and PoW-only.
2. **Scalability through layering**: Move high-throughput payments and smart-contract execution to upper layers.
3. **Deterministic finality semantics**: Use >=2/3 stake BFT signatures for DRACHMA and OBOLOS finality.
4. **Operational clarity**: Define explicit responsibilities for validators, miners, relayers, and bridge operators.

---

## 2. System Architecture

### 2.1 Layer roles

#### TALANTON — Layer-1 (PoW)

- Consensus: SHA-256d Proof-of-Work
- Responsibilities:
  - final settlement
  - immutable anchor for higher layers
  - root of trust
- Native token: **TALANTON** (mining rewards and L1 fees only)
- No staking
- Accepts and validates `TX_L2_COMMIT` commitments from DRACHMA

#### DRACHMA — Layer-2 (PoS Payments)

- Consensus: Epoch-based PoS with BFT finality
- Finality threshold: >=2/3 active stake signatures
- Responsibilities:
  - fast payments
  - liquidity and transfer throughput
  - minimal execution (no general EVM)
- Native token: **DRACHMA** (staking + fees)
- Accepts `TX_L3_COMMIT` from OBOLOS
- Publishes `TX_L2_COMMIT` to TALANTON

#### OBOLOS — Layer-3 (PoS Execution)

- Consensus: Epoch-based PoS with BFT finality
- Finality threshold: >=2/3 active stake signatures
- Responsibilities:
  - EVM execution
  - gas accounting
- Native token: **OBOLOS** (gas + staking)
- Publishes finalized commitments to DRACHMA

### 2.2 Canonical anchoring path

`OBOLOS -> DRACHMA -> TALANTON`

DRACHMA commitments include the latest finalized OBOLOS commitment hash to preserve transitive anchoring.

---

## 3. Commitment Transactions

### 3.1 `TX_L2_COMMIT` (on TALANTON)

`source_chain = DRACHMA`

Required fields:

- `epoch`
- `finalized_height`
- `finalized_block_hash`
- `state_root`
- `validator_set_hash`
- `validator_signatures`

Validation requirements on TALANTON:

1. monotonic finalized height
2. valid payload encoding
3. quorum signatures >=2/3 active DRACHMA stake

### 3.2 `TX_L3_COMMIT` (on DRACHMA)

`source_chain = OBOLOS`

Required fields mirror `TX_L2_COMMIT`:

- `epoch`
- `finalized_height`
- `finalized_block_hash`
- `state_root`
- `validator_set_hash`
- `validator_signatures`

Validation requirements on DRACHMA:

1. monotonic finalized height
2. valid payload encoding
3. quorum signatures >=2/3 active OBOLOS stake

---

## 4. Consensus Specifications

### 4.1 TALANTON PoW

TALANTON uses pure SHA-256d mining and heaviest-chain settlement semantics. Staking logic is disallowed by design.

### 4.2 DRACHMA and OBOLOS PoS

Both PoS layers implement:

- epoch-based operation
- deterministic proposer selection
- validator sets derived from on-chain staking state
- slashing for:
  - double-signing
  - equivocation
- finality after >=2/3 active stake signatures

---

## 5. Bridging and Asset Flow (MVP)

### 5.1 L1 <-> L2

- **Deposit**: lock on TALANTON -> mint wrapped representation on DRACHMA
- **Withdraw**: burn on DRACHMA -> unlock on TALANTON

### 5.2 L2 <-> L3

- **Deposit**: lock on DRACHMA -> mint wrapped representation on OBOLOS
- **Withdraw**: burn on OBOLOS -> unlock on DRACHMA

### 5.3 MVP properties

Withdrawals are optimistic and subject to a trust window. The bridge model is economic and operational, not validity-proof based.

---

## 6. Token Economics

### TALANTON

- mining rewards (PoW)
- L1 fees
- no staking utility

### DRACHMA

- PoS inflation rewards
- L2 fees
- validator staking collateral

### OBOLOS

- PoS inflation rewards
- EVM gas fees
- validator staking collateral

---

## 7. Configuration and Genesis

Network initialization is layer-specific:

- `genesis_talanton.json`
- `genesis_drachma.json`
- `genesis_obolos.json`

Genesis parameters include:

- inflation schedules
- epoch lengths
- minimum stake
- slashing ratios
- commitment intervals

---

## 8. Node, CLI, and RPC

### Node mode

`pantheon-node --layer=l1|l2|l3`

### CLI examples

- `pantheon-cli stake deposit --layer=l2`
- `pantheon-cli deploy-contract --layer=l3`
- `pantheon-cli submit-commitment --layer=l2|l3`

### RPC namespaces

- `/chain/info`
- `/staking/*`
- `/commitments/*`
- `/evm/*` (OBOLOS only)

---

## 9. Testing and Operations

Required testing domains:

- Unit tests:
  - staking and slashing
  - proposer determinism
  - commitment validation
- Integration tests:
  - L1+L2+L3 local orchestration
  - anchoring verification (OBOLOS -> DRACHMA -> TALANTON)
  - withdrawal flow verification
- Fuzz testing:
  - transaction decoding
  - consensus message parsing

CI includes build, lint, and test gates.

---

## 10. Security and Trust Assumptions (MVP)

PantheonChain MVP does **not** include zk proofs or fraud proofs by default.

Security assumptions:

1. TALANTON PoW remains economically secure against majority hash attacks.
2. DRACHMA and OBOLOS finality is safe when <1/3 active stake is Byzantine.
3. Relayers are required for commitment liveness but do not define consensus safety.
4. Bridge withdrawals are optimistic and should be treated as delayed-finality operations.

These assumptions and limitations are intentionally explicit and operator-visible.

---

## 11. Conclusion

PantheonChain’s layered model decouples security, payments, and execution while preserving an auditable trust chain from OBOLOS through DRACHMA into TALANTON. The architecture is intentionally modular, enabling independent optimization of each layer without weakening the settlement anchor.
