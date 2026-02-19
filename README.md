# PantheonChain

![Build](https://github.com/Tsoympet/PantheonChain/actions/workflows/build-test-devnet.yml/badge.svg)
![Lint](https://github.com/Tsoympet/PantheonChain/actions/workflows/lint.yml/badge.svg)

PantheonChain is a layered blockchain stack with explicit separation of settlement security, payments, and smart-contract execution.

## Layers and Tokens

- **TALANTON (L1)**: PoW (SHA-256d) security anchor and final settlement. Token used for mining rewards and L1 fees.
- **DRACHMA (L2)**: PoS+BFT payments/liquidity layer. Token used for staking and L2 fees.
- **OBOLOS (L3)**: PoS+BFT execution layer with EVM environment. Token used for gas and staking.

Canonical anchoring path: `OBOLOS -> DRACHMA -> TALANTON`.

## Repository Layout

- `src/common`, `src/talanton`, `src/drachma`, `src/obolos`, `src/relayers`, `src/tools`
- `configs/{devnet,testnet,mainnet}`
- `docs/`
- `scripts/`
- `tests/{unit,integration,fixtures}`

## Quickstart (devnet)

```bash
./scripts/build.sh
./scripts/run-devnet.sh
./scripts/test.sh
```

For Docker-based startup:

```bash
docker compose up --build
```

## Quickstart (testnet)

```bash
./scripts/build.sh
./scripts/run-testnet.sh
./tests/integration/testnet-smoke.sh
```

Testnet uses ports 28332/29332/30332 (RPC) and 28333/29333/30333 (P2P).
See [`configs/testnet/README.md`](configs/testnet/README.md) for configuration details.

## Documentation

- [Architecture](docs/architecture.md)
- [Build](docs/build.md)
- [Run Devnet](docs/run-devnet.md)
- [RPC](docs/rpc.md)
- [CLI](docs/cli.md)
- [Tokenomics](docs/tokenomics.md)
- [Threat Model](docs/threat-model.md)
- [Migration](docs/migration.md)
- [Glossary](docs/glossary.md)
- [Repository Audit Report](docs/repo-audit-report.md)

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).
