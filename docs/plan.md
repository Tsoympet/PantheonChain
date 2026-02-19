# Execution Plan

1. **Shared core extraction (`src/common`)**
   - Consolidate commitment schemas, encoding, p2p helpers, cryptography, storage, mempool, and metrics.

2. **TALANTON hardening (`src/talanton`)**
   - Keep PoW security role unchanged.
   - Enforce strict `TX_L2_COMMIT` monotonicity, payload checks, and quorum validation.

3. **DRACHMA PoS completion (`src/drachma`)**
   - Deterministic proposer selection.
   - Staking-aware active set accounting.
   - Slashing handlers for double-signing and equivocation.
   - `TX_L3_COMMIT` validation pipeline.

4. **OBOLOS execution layer (`src/obolos`)**
   - EVM-like execution and gas accounting primitives.
   - Finalized checkpoint generation for DRACHMA anchoring.

5. **Commitment relayers (`relayers/`)**
   - Implement runnable relayers for OBOLOS -> DRACHMA and DRACHMA -> TALANTON.

6. **Interfaces**
   - Maintain layer-aware node mode (`pantheon-node --layer=l1|l2|l3`).
   - Maintain CLI staking/deploy/commitment workflows by layer.

7. **Verification**
   - Expand unit coverage for proposer determinism, slashing, commitment validation, and shared utilities.
   - Run local integration smoke for layered anchoring path.
