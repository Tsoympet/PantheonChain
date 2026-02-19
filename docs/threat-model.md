# Threat Model

## Assumptions (MVP)

- L2/L3 finality assumes >=2/3 honest active stake.
- Commitments are rollup-style with signatures; no zk/fraud proofs in MVP.
- Relayers may delay but not silently rewrite finalized history when quorum assumptions hold.

## Operational risks

- Exposed unauthenticated RPC is for devnet only.
- Validator/relayer key compromise can impact liveness and safety.
