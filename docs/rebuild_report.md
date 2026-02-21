# Rebuild Report

## Final tree snapshot (top-level)

- layer1-talanton
- layer2-drachma
- layer3-obolos
- common
- relayers
- cli
- configs
- docs
- scripts
- tests
- docker
- include
- clients
- enterprise
- installers
- assets
- third_party
- tools

## Legacy absorption summary

Legacy `src/*` layer modules were moved into canonical layer directories. Shared primitives were moved into `common/*`. `legacy/` and `src/` were removed after absorbing remaining functionality into canonical modules.

## Build / test / devnet

- `scripts/build.sh`
- `scripts/test.sh`
- `scripts/run-devnet.sh`

## Known limitations and next steps

- Current devnet uses a mock RPC harness for smoke validation.
- Additional directory normalization is still desirable for strict one-to-one mapping between
  auxiliary trees (`clients`, `installers`, `tools`, `assets`) and canonical module ownership.
- Integration smoke currently validates mocked/devnet orchestration paths; production-hardening
  scenarios should be extended in `tests/integration`.
- Continue migrating `layer1-talanton/core/*` internals into the strict phase-1 layout
  (`node/consensus/ledger/tx/rpc`) while preserving API stability.
- Additional production consensus/VM hardening remains ongoing.
