# Threat Model (MVP)

## Security Model

PantheonChain MVP uses rollup-style commitments with economic-finality signatures.

- No zk-proofs.
- No fraud proofs.
- Security depends on honest-majority-by-stake (>=2/3) assumptions in DRACHMA and OBOLOS validator sets.

## Trust and Finality Assumptions

- TALANTON provides PoW immutability for final settlement.
- DRACHMA and OBOLOS provide fast finality under BFT quorum assumptions.
- Relayers are responsible for liveness of commitment publication, not consensus safety.

## Primary Risks

- Validator key compromise can break safety/liveness on L2/L3.
- Relayer censorship or delay can postpone anchoring.
- Optimistic bridge withdrawals inherit trust-window risk until final settlement confidence.

## MVP Limitations

- Commitment validation is signature/quorum-based only.
- Bridges are economically secure but not cryptographically trustless.
- Operators should monitor commitment freshness along `OBOLOS -> DRACHMA -> TALANTON`.


## Bridge-specific MVP assumptions

- Withdrawals are optimistic and enforce a configurable trust window before unlock.
- Safety relies on honest-majority finality signatures, not validity proofs.
- Operators should treat bridge unlocks as delayed-finality operations tied to commitment freshness.
