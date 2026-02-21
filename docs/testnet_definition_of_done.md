# Testnet Definition of Done

## Acceptance Criteria

- Clean checkout builds on Ubuntu CI via `./scripts/build.sh`.
- Unit tests pass via `ctest --test-dir build --output-on-failure`.
- Integration devnet smoke passes via `./scripts/run-devnet.sh && ./tests/integration/devnet-smoke.sh`.
- RPC health endpoints respond on all layers:
  - TALANTON L1: `GET /health`, `GET /chain/info`
  - DRACHMA L2: `GET /health`, `GET /chain/info`
  - OBOLOS L3: `GET /health`, `GET /chain/info`
- Commitment anchoring verified end-to-end:
  - OBOLOS commitment appears on DRACHMA (`commitments/get id=l2-anchor-1`)
  - DRACHMA commitment appears on TALANTON (`commitments/get id=l1-anchor-1`)
  - TALANTON commitment references latest OBOLOS commitment hash (`references[]` includes OBOLOS hash)
- CLI supports testnet/devnet operations:
  - L2 transfer send (`transfer send --layer=l2`)
  - L3 contract deploy/call (`contract deploy|call --layer=l3`)
  - Commitment query (`commitments list|get --layer=l1|l2`)
- Placeholder gate passes for protected paths.

## CI Gate

`./scripts/placeholder-gate.sh` must pass and fails on any protected path containing:

- `TODO(PROD)`
- `PLACEHOLDER`
- `DUMMY`
- `MOCK`
- `FIXME(PROD)`
