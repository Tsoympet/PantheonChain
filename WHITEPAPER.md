# PantheonChain Whitepaper

**Version 2.0 (Layered Refactor Edition)**  
**Date: January 2026**

---

## Abstract

PantheonChain is a modular blockchain stack that separates security, payments, and execution into three anchored layers:

- **TALANTON (L1)** for Proof-of-Work settlement and root security.
- **DRACHMA (L2)** for Proof-of-Stake payments and liquidity.
- **OBOLOS (L3)** for Proof-of-Work EVM execution.

The system uses rollup-style commitment anchoring with hash-power quorum signatures on PoW layers. The canonical commitment path is **OBOLOS -> DRACHMA -> TALANTON**. This whitepaper defines the architecture, consensus model, commitment transaction formats, bridging flows, and MVP trust assumptions.

---

## 1. Design Goals

1. **Security minimization at L1**: Keep the settlement layer simple, immutable, and PoW-only.
2. **Scalability through layering**: Move high-throughput payments and smart-contract execution to upper layers.
3. **Deterministic finality semantics**: Use >=2/3 hash-power quorum signatures for DRACHMA and OBOLOS finality.
4. **Operational clarity**: Define explicit responsibilities for miners, relayers, and bridge operators.

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

#### DRACHMA — Layer-2 (PoW Payments)

- Consensus: Epoch-based Proof-of-Work with hash-power quorum finality
- Finality threshold: >=2/3 contributing hash-power signatures
- Responsibilities:
  - fast payments
  - liquidity and transfer throughput
  - minimal execution (no general EVM)
- Native token: **DRACHMA** (mining rewards + fees)
- No staking
- Accepts `TX_L3_COMMIT` from OBOLOS
- Publishes `TX_L2_COMMIT` to TALANTON

#### OBOLOS — Layer-3 (PoW Execution)

- Consensus: Epoch-based Proof-of-Work with hash-power quorum finality
- Finality threshold: >=2/3 contributing hash-power signatures
- Responsibilities:
  - EVM execution
  - gas accounting
- Native token: **OBOLOS** (gas + mining rewards)
- No staking
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
- `miner_set_hash`
- `miner_signatures`

Validation requirements on TALANTON:

1. monotonic finalized height
2. valid payload encoding
3. quorum signatures >=2/3 contributing DRACHMA hash power

### 3.2 `TX_L3_COMMIT` (on DRACHMA)

`source_chain = OBOLOS`

Required fields mirror `TX_L2_COMMIT`:

- `epoch`
- `finalized_height`
- `finalized_block_hash`
- `state_root`
- `miner_set_hash`
- `miner_signatures`

Validation requirements on DRACHMA:

1. monotonic finalized height
2. valid payload encoding
3. quorum signatures >=2/3 contributing OBOLOS hash power

---

## 4. Consensus Specifications

### 4.1 TALANTON PoW

TALANTON uses pure SHA-256d mining and heaviest-chain settlement semantics. Staking logic is disallowed by design.

### 4.2 DRACHMA and OBOLOS PoW

All three layers use Proof-of-Work:

- epoch-based operation
- deterministic proposer selection weighted by hash power
- miner sets derived from on-chain PoW contribution
- no slashing (miners are penalised by orphaned blocks)
- finality after >=2/3 contributing hash-power signatures

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

PantheonChain has three native assets with independent issuance schedules.
All three use Bitcoin-style geometric halvings (interval: 210,000 blocks ≈ 4 years).

### Supply caps and issuance

| Asset       | Ticker | Hard cap          | Achievable supply | Initial reward      | Launch height |
|-------------|--------|------------------:|------------------:|:-------------------:|:-------------:|
| TALANTON    | TALN   | 21,000,000        | ~21,000,000       | 50 TALN/block       | block 0       |
| DRACHMA     | DRM    | 100,000,000,000   | ~99,960,000,000   | 238,000 DRM/block   | block 210,000 |
| OBOLOS      | OBL    | 100,000,000,000   | ~99,960,000,000   | 238,000 OBL/block   | block 420,000 |

**Hard cap** – the strict consensus limit; no coinbase can push the circulating supply above it.  
**Achievable supply** – `initial_reward × 210,000 × 2`; the actual ceiling the halving
schedule can reach (integer right-shift means the issuance approaches this asymptotically).

TALANTON's cap and achievable supply are essentially identical (gap < 0.001 TALN).  
DRACHMA and OBOLOS hard caps are set to **100,000,000,000** (100 billion), matching the XRP maximum supply.  
Achievable supply for both DRM and OBL is ~99.96B (40M below the 100B hard cap).

Governance quorum is expressed as a percentage of the total number of eligible voters
(token holders), not of the token supply.

### Governance — One-Address-One-Vote

PantheonChain governance uses **one-address-one-vote (1A1V)**:

- Every address holding ≥ 1 token of any asset gets exactly **1 vote**.
- The amount of tokens held has no effect on voting power.
- A holder with 1 TALN and a holder with 100 billion DRM each cast votes of equal weight.
- Quorum thresholds are percentages of the total eligible voter count.
- Snapshots are taken at `voting_start` block; addresses that acquire tokens later cannot vote on that proposal.

### TALANTON

- mining rewards (PoW)
- L1 fees
- no staking utility; governance voting via 1A1V

### DRACHMA

- PoW mining rewards
- L2 fees
- no staking utility; governance voting via 1A1V

### OBOLOS

- PoW mining rewards
- EVM gas fees
- no staking utility; governance voting via 1A1V

---

## 7. Configuration and Genesis

Network initialization is layer-specific:

- `genesis_talanton.json`
- `genesis_drachma.json`
- `genesis_obolos.json`

Genesis parameters include:

- inflation schedules
- epoch lengths
- commitment intervals

---

## 8. Node, CLI, and RPC

### Node mode

`pantheon-node --layer=l1|l2|l3`

### CLI examples

- `pantheon-cli mine --layer=l2`
- `pantheon-cli deploy-contract --layer=l3`
- `pantheon-cli submit-commitment --layer=l2|l3`
- `pantheon-cli governance vote --proposal-id=<id>`

### RPC namespaces

- `/chain/info`
- `/governance/*` (one-address-one-vote; no staking required)
- `/commitments/*`
- `/evm/*` (OBOLOS only)

---

## 9. Testing and Operations

Required testing domains:

- Unit tests:
  - one-address-one-vote (1A1V) voting power
  - proposer determinism (PoW hash-power weighted)
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
2. DRACHMA and OBOLOS finality is safe when <1/3 of total contributing hash power is Byzantine.
3. Relayers are required for commitment liveness but do not define consensus safety.
4. Bridge withdrawals are optimistic and should be treated as delayed-finality operations.

These assumptions and limitations are intentionally explicit and operator-visible.

---

## 11. Conclusion

PantheonChain’s layered model decouples security, payments, and execution while preserving an auditable trust chain from OBOLOS through DRACHMA into TALANTON. The architecture is intentionally modular, enabling independent optimization of each layer without weakening the settlement anchor.
