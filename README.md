# PantheonChain

[![Build Test Devnet](https://github.com/Tsoympet/PantheonChain/actions/workflows/build-test-devnet.yml/badge.svg)](https://github.com/Tsoympet/PantheonChain/actions/workflows/build-test-devnet.yml)
[![Build](https://github.com/Tsoympet/PantheonChain/actions/workflows/build.yml/badge.svg)](https://github.com/Tsoympet/PantheonChain/actions/workflows/build.yml)

PantheonChain is organized into strict canonical layers:

- **Layer 1 / TALANTON** (`layer1-talanton`): PoW anchor chain.
- **Layer 2 / DRACHMA** (`layer2-drachma`): PoS payments layer.
- **Layer 3 / OBOLOS** (`layer3-obolos`): PoS + EVM execution layer.

Anchoring path: **OBOLOS -> DRACHMA -> TALANTON**.

## Quickstart

```bash
./scripts/build.sh
./scripts/run-devnet.sh
./tests/integration/devnet-smoke.sh
```

## Devnet/Testnet

- Devnet configs: `configs/devnet/`
- Testnet template configs: `configs/testnet/`
- Config validation:

```bash
python3 scripts/validate-config.py configs/devnet/l1.json configs/devnet/l2.json configs/devnet/l3.json
```

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

## Operator docs

- [Testnet Definition of Done](docs/testnet_definition_of_done.md)
- [Testnet Ready Report](docs/testnet_ready_report.md)
- [Testnet Ops Runbook](docs/ops/testnet_operations_runbook.md)

## Existing docs

- [Architecture](docs/architecture.md)
- [Build](docs/build.md)
- [Run devnet](docs/run-devnet.md)
- [RPC](docs/rpc.md)
- [CLI](docs/cli.md)
