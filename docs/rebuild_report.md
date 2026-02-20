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

## Legacy absorption summary

Legacy `src/*` layer modules were moved into canonical layer directories. Shared primitives were moved into `common/*`. `legacy/` was removed after absorbing remaining functionality into canonical modules.

## Build / test / devnet

- `scripts/build.sh`
- `scripts/test.sh`
- `scripts/run-devnet.sh`

## Known limitations

- Current devnet uses a mock RPC harness for smoke validation.
- Additional production consensus/VM hardening remains ongoing.
