# RPC by Layer

PantheonChain uses JSON-RPC over HTTP `POST /`.

## Shared

- `/chain/info`

## DRACHMA (L2) and OBOLOS (L3)

- `/staking/deposit`
- `/staking/validators`

## Commitments

- `/commitments/submit`
- `/commitments/list`

## OBOLOS-only

- `/evm/deploy`
- `/evm/call`

Canonical anchoring path is `OBOLOS -> DRACHMA -> TALANTON`; therefore `/commitments/submit` on L2 accepts `TX_L3_COMMIT`, while on L1 it accepts `TX_L2_COMMIT`.
