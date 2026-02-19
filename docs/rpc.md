# RPC by Layer

PantheonChain uses JSON-RPC over HTTP `POST /`.

## Shared/Node

- `getinfo`
- `getblockcount`
- `getbalance`
- `sendtoaddress`
- `stop`
- Health endpoint: `GET /health`

## Layered aliases

- `/chain/info` -> method `chain/info`
- `/staking/*` -> method `staking/deposit`
- `/commitments/*` -> methods `commitments/submit`, `commitments/list`
- `/evm/*` -> method `evm/deploy` (OBOLOS mode)
