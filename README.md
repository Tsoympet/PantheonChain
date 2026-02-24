# PantheonChain

[![Build Test Devnet](https://github.com/Tsoympet/PantheonChain/actions/workflows/build-test-devnet.yml/badge.svg)](https://github.com/Tsoympet/PantheonChain/actions/workflows/build-test-devnet.yml)
[![Build Test Testnet](https://github.com/Tsoympet/PantheonChain/actions/workflows/build-test-testnet.yml/badge.svg)](https://github.com/Tsoympet/PantheonChain/actions/workflows/build-test-testnet.yml)
[![Build](https://github.com/Tsoympet/PantheonChain/actions/workflows/build.yml/badge.svg)](https://github.com/Tsoympet/PantheonChain/actions/workflows/build.yml)
[![Tests](https://github.com/Tsoympet/PantheonChain/actions/workflows/test.yml/badge.svg)](https://github.com/Tsoympet/PantheonChain/actions/workflows/test.yml)
[![Lint](https://github.com/Tsoympet/PantheonChain/actions/workflows/lint.yml/badge.svg)](https://github.com/Tsoympet/PantheonChain/actions/workflows/lint.yml)
[![Security](https://github.com/Tsoympet/PantheonChain/actions/workflows/security.yml/badge.svg)](https://github.com/Tsoympet/PantheonChain/actions/workflows/security.yml)

**PantheonChain** is a production-grade, three-layer blockchain inspired by ancient Athenian democracy — combining Proof-of-Work settlement, Proof-of-Stake payments, and EVM-compatible smart contract execution with an on-chain governance system rooted in Greek democratic principles.

## Architecture

PantheonChain is organized into three canonical layers:

| Layer | Name | Consensus | Purpose |
|-------|------|-----------|---------|
| Layer 1 | **TALANTON** (`layer1-talanton`) | PoW | Immutable anchor chain — final settlement |
| Layer 2 | **DRACHMA** (`layer2-drachma`) | PoS | High-throughput payments and Plasma rollup |
| Layer 3 | **OBOLOS** (`layer3-obolos`) | PoS + EVM | Smart contract execution, governance, DeFi |

Anchoring path: **OBOLOS → DRACHMA → TALANTON**.

## Quickstart

```bash
# Build all layers
./scripts/build.sh

# Launch a local devnet
./scripts/run-devnet.sh

# Run integration smoke tests
./tests/integration/devnet-smoke.sh
```

## Devnet / Testnet

- Devnet configs: `configs/devnet/`
- Testnet configs: `configs/testnet/`
- Config validation:

```bash
python3 scripts/validate-config.py configs/devnet/l1.json configs/devnet/l2.json configs/devnet/l3.json
```

## Governance

PantheonChain features an on-chain governance system inspired by Athenian democracy. Governance is implemented across the Layer 3 (OBOLOS) smart contract layer and enforced via VRF sortition, snapshot voting, and constitutional parameter limits.

### Key Institutions

| Institution | Greek Term | Role |
|-------------|-----------|------|
| **Council** | *Boule* | VRF-selected validator committee; proposes and filters protocol changes |
| **Assembly** | *Ekklesia* | All stakers vote on proposals; sovereign decision body |
| **Executive Committee** | *Prytany* | Rotating subset of Boule; fast-tracks emergency actions |
| **Eligibility Check** | *Dokimasia* | On-chain screening of council candidates |
| **Community Ban** | *Ostracism* | Time-limited exclusion of malicious actors by assembly vote |
| **Investigative Board** | *Apophasis* | Reviews and ratifies Emergency Council actions |
| **Equal Law** | *Isonomia* | Hard-coded constitutional floors and ceilings no vote can override |

### Proposal Types

- **STANDARD** — Simple majority, 7-day voting window
- **CONSTITUTIONAL** — 66 % supermajority, 14-day window (required to amend the constitution)
- **EMERGENCY** — Prytany fast-track (48 h), requires subsequent Apophasis ratification
- **PARAMETER_CHANGE** — Adjusts protocol parameters within Isonomia bounds
- **TREASURY_SPENDING** — Allocates funds from one of five treasury tracks

See [docs/CONSTITUTION.md](docs/CONSTITUTION.md) for the full Governance Constitution.

## Repository Structure

```
PantheonChain/
├── layer1-talanton/        # PoW anchor chain
├── layer2-drachma/         # PoS payments + Plasma
├── layer3-obolos/          # PoS + EVM + Governance
├── common/                 # Shared cryptographic primitives
├── relayers/
│   ├── relayer-l2/         # L1↔L2 bridge
│   └── relayer-l3/         # L2↔L3 bridge
├── cli/pantheon-cli/       # Unified command-line interface
├── configs/
│   ├── devnet/             # Local development network
│   ├── testnet/            # Public testnet
│   └── mainnet/            # Mainnet (future)
├── docs/                   # Documentation
├── scripts/                # Build and operation scripts
├── tests/                  # Unit, integration, and consensus tests
└── docker/                 # Container definitions
```

## Documentation

### Operations
- [Testnet Definition of Done](docs/testnet_definition_of_done.md)
- [Testnet Ready Report](docs/testnet_ready_report.md)
- [Testnet Ops Runbook](docs/ops/testnet_operations_runbook.md)
- [Operations Runbook](docs/OPERATIONS_RUNBOOK.md)
- [Runbooks: Backup & Restore, Key Compromise, Incident Response](docs/runbooks/)

### Architecture & Protocol
- [Architecture Overview](docs/architecture.md)
- [Consensus](docs/consensus.md)
- [Layer 1 Core](docs/LAYER1_CORE.md)
- [Layer 2 Protocols](docs/LAYER2_PROTOCOLS.md)
- [Tokenomics](docs/tokenomics.md)
- [Denominations](docs/denominations.md)

### Governance
- [Constitution](docs/CONSTITUTION.md)
- [Glossary](docs/glossary.md)

### Developer
- [Build Guide](docs/build.md)
- [Run Devnet](docs/run-devnet.md)
- [RPC Reference](docs/rpc.md)
- [CLI Reference](docs/cli.md)
- [Security Model](docs/SECURITY_MODEL.md)
- [Threat Model](docs/threat-model.md)

### Releases
- [Changelog](CHANGELOG.md)
- [Roadmap](docs/ROADMAP.md)
- [Releases](docs/RELEASES.md)
