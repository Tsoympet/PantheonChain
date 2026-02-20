# PantheonChain

PantheonChain is organized into strict canonical layers:

- **Layer 1 / TALANTON** (`layer1-talanton`): PoW anchor chain.
- **Layer 2 / DRACHMA** (`layer2-drachma`): PoS+BFT payments layer.
- **Layer 3 / OBOLOS** (`layer3-obolos`): PoS+BFT execution layer with EVM and gas in OBOLOS.

Anchoring path: **OBOLOS -> DRACHMA -> TALANTON**.

## Repository structure

- `layer1-talanton/`
- `layer2-drachma/`
- `layer3-obolos/`
- `common/`
- `relayers/relayer-l2`, `relayers/relayer-l3`
- `cli/pantheon-cli`
- `configs/{devnet,testnet,mainnet}`
- `docs/`
- `scripts/`
- `tests/`
- `docker/`

## Build

```bash
./scripts/build.sh
```

## Test

```bash
./scripts/test.sh
```

## Run devnet

```bash
./scripts/run-devnet.sh
./tests/integration/devnet_smoke_test
```

## Docs

- [Architecture](docs/architecture.md)
- [Build](docs/build.md)
- [Run devnet](docs/run-devnet.md)
- [RPC](docs/rpc.md)
- [CLI](docs/cli.md)
- [Tokenomics](docs/tokenomics.md)
- [Threat model](docs/threat-model.md)
- [Migration](docs/migration.md)
- [Troubleshooting](docs/troubleshooting.md)
- [Glossary](docs/glossary.md)
- [Rebuild absorption plan](docs/rebuild_absorption_plan.md)
- [Rebuild report](docs/rebuild_report.md)
