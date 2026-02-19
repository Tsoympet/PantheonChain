# Threat Model

## Assumptions (MVP)

- L2/L3 finality assumes >=2/3 honest active stake.
- Commitments are rollup-style with signatures; no zk/fraud proofs in MVP.
- Relayers may delay but not silently rewrite finalized history when quorum assumptions hold.

## Operational risks

- Exposed unauthenticated RPC is for devnet only.
- Validator/relayer key compromise can impact liveness and safety.
# Threat Model (MVP)

## Security assumptions

This MVP uses rollup-style commitments with economic finality signatures.

- No zk-proofs.
- No fraud proofs.
- Trust model depends on >=2/3 honest active stake in L2 and L3 validator sets.

## Known limitations

- Bridge withdrawals are optimistic with a trust window.
- Commitment validity is signature/quorum based, not proof-system based.
- Relayer censorship/delay is possible but detectable from missing commitments.
