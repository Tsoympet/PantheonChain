# Implementation Plan

1. Refactor code into shared core (`src/common`) and layer modules (`src/talanton`, `src/drachma`, `src/obolos`).
2. Stabilize TALANTON L1 commitment validation (`TX_L2_COMMIT`).
3. Implement DRACHMA PoS proposer/slashing/commitment validation (`TX_L3_COMMIT`).
4. Implement OBOLOS execution and gas accounting primitives.
5. Add relayers for `DRACHMA->TALANTON` and `OBOLOS->DRACHMA`.
6. Add CLI and node layer mode interfaces.
7. Add unit tests and CI verification.
