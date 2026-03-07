# PantheonChain

[![Build Test Devnet](https://github.com/Tsoympet/PantheonChain/actions/workflows/build-test-devnet.yml/badge.svg)](https://github.com/Tsoympet/PantheonChain/actions/workflows/build-test-devnet.yml)
[![Build Test Testnet](https://github.com/Tsoympet/PantheonChain/actions/workflows/build-test-testnet.yml/badge.svg)](https://github.com/Tsoympet/PantheonChain/actions/workflows/build-test-testnet.yml)
[![Build](https://github.com/Tsoympet/PantheonChain/actions/workflows/build.yml/badge.svg)](https://github.com/Tsoympet/PantheonChain/actions/workflows/build.yml)
[![Tests](https://github.com/Tsoympet/PantheonChain/actions/workflows/test.yml/badge.svg)](https://github.com/Tsoympet/PantheonChain/actions/workflows/test.yml)
[![Lint](https://github.com/Tsoympet/PantheonChain/actions/workflows/lint.yml/badge.svg)](https://github.com/Tsoympet/PantheonChain/actions/workflows/lint.yml)
[![Security](https://github.com/Tsoympet/PantheonChain/actions/workflows/security.yml/badge.svg)](https://github.com/Tsoympet/PantheonChain/actions/workflows/security.yml)

**PantheonChain** is a hybrid layered blockchain network with strict consensus-role separation:
- **TALANTON (L1)** is the only consensus-critical PoW settlement chain.
- **DRACHMA (L2)** is a PoS/BFT payments layer with fast economic finality.
- **OBOLOS (L3)** is a PoS/BFT EVM execution and governance layer.

## Architecture

PantheonChain is organized into three canonical layers:

| Layer | Name | Consensus | Purpose |
|-------|------|-----------|---------|
| Layer 1 | **TALANTON** (`layer1-talanton`) | PoW | Root settlement and anchoring chain |
| Layer 2 | **DRACHMA** (`layer2-drachma`) | PoS + BFT | High-throughput payments and intermediate checkpoint layer |
| Layer 3 | **OBOLOS** (`layer3-obolos`) | PoS + BFT + EVM | Smart contract execution, governance, and DeFi |

Canonical checkpoint path: **OBOLOS → DRACHMA → TALANTON**.

Upper-layer finality (OBOLOS/DRACHMA) is **economic finality** under validator-honesty assumptions. TALANTON provides **ultimate settlement** with PoW probabilistic finality and is the root of trust for the stack.

## Consensus and Settlement Model

### Layer consensus roles
- **TALANTON (L1):** SHA-256d PoW chain. It defines the strongest settlement guarantees and does not depend on L2/L3 validators for its own consensus safety.
- **DRACHMA (L2):** PoS/BFT payment rail. It finalizes quickly under validator quorum assumptions and checkpoints finalized L2 state to TALANTON.
- **OBOLOS (L3):** PoS/BFT EVM layer. It finalizes execution quickly under validator quorum assumptions and checkpoints to DRACHMA.

### Token security scope
- **TALANTON token:** PoW mining reward asset, L1 fee/settlement asset, and base security token.
- **DRACHMA token:** L2 staking collateral and fee utility for the payments layer.
- **OBOLOS token:** L3 staking, gas, governance, and execution-economy asset.

### Finality semantics
- **OBOLOS and DRACHMA:** fast economic finality, reversible under severe validator corruption or operational failure.
- **TALANTON:** probabilistic PoW settlement finality; strongest assurance boundary in the system.

### Trust assumptions
- Bridge/checkpoint safety on L2/L3 depends on honest-majority validator assumptions unless stronger proof systems are introduced.
- Relayers are liveness components for publishing commitments; they are not a replacement for base settlement.

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
- Config validation (schema + canonical layer model from `configs/layer-model.json`):

```bash
python3 scripts/validate-config.py configs/devnet/l1.json configs/devnet/l2.json configs/devnet/l3.json
python3 scripts/validate-layer-model.py
```

## Governance

PantheonChain features an on-chain governance system inspired by Athenian democracy. Governance is implemented primarily on the OBOLOS execution layer and coordinated with protocol-level controls.

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

See [docs/CONSTITUTION.md](docs/CONSTITUTION.md) for the full Governance Constitution.

## Repository Structure

```
PantheonChain/
├── layer1-talanton/        # PoW root-settlement chain
├── layer2-drachma/         # PoS/BFT payments + checkpointing to L1
├── layer3-obolos/          # PoS/BFT + EVM execution + checkpointing to L2
├── common/                 # Shared cryptographic primitives
├── relayers/
│   ├── relayer-l2/         # DRACHMA -> TALANTON commitment relay
│   └── relayer-l3/         # OBOLOS -> DRACHMA commitment relay
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
- [Settlement and Finality](docs/SETTLEMENT_AND_FINALITY.md)
- [Architecture Alignment Gaps & Next Steps](docs/ARCHITECTURE_ALIGNMENT_GAPS.md)
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
