# RPC by Layer

PantheonChain uses JSON-RPC over HTTP `POST /`.

## Shared

- `/chain/info`
- `/chain/monetary_spec` (returns denomination constants + `MonetarySpecHash`)

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

## Monetary and Amount Encoding

RPC amount payloads expose both raw and formatted values:

```json
{
  "amount_raw": "123456789",
  "amount": "1.23456789",
  "token": "DRACHMA"
}
```

Fixed protocol denomination constants:

- `1 DRACHMA = 6 OBOLOS`
- `1 TALANTON = 6000 DRACHMA = 36000 OBOLOS`

These are protocol-level unit-of-account conversions only (not price pegs).


## Denomination Override

Relevant amount endpoints accept an optional denomination override through params (JSON-RPC style equivalent of `?denom=<name>`).

Example request params for `getbalance`:

```json
["DRACHMA", "tetradrachm"]
```

Responses include:

- `amount_raw`
- `amount_formatted`
- `denom_used`

This is presentation-only and does not change on-chain accounting.
