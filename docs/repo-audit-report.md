# Repository Audit Report

## Gap scan summary

### Missing or inconsistent before this pass

- Mainnet profile templates were incomplete (`configs/mainnet` lacked `l1/l2/l3` JSON+CONF templates).
- Devnet daemon config files had stale network mode values.
- Audit script checks were too narrow (did not verify all required layered directories/profiles).
- Config validation covered basic shape only and did not catch profile/path mismatches or duplicate ports.
- Root README runtime examples still referenced old `pantheon-node` naming.
- Legacy compatibility scope needed clearer guidance for old roots (`layer1`, `layer2`, top-level `tools`).
- Devnet/testnet startup scripts returned before RPC listeners were ready, causing intermittent smoke-test failures from clean checkouts.

### Added/updated in this pass

- Added full `configs/mainnet/{l1,l2,l3}.json` and `configs/mainnet/{l1,l2,l3}.conf` templates.
- Corrected `configs/devnet/*.conf` to use `network.mode=devnet`.
- Expanded `scripts/validate-config.py` with network checks, stronger field validation, and cross-file port conflict detection.
- Expanded `scripts/repo-audit.sh` to validate required layered directories, required docs/configs/workflows, executable scripts, and stale docs references.
- Updated `README.md` with daemon-accurate run commands, expanded docs index, and mermaid anchoring diagram.
- Updated `CONTRIBUTING.md` with explicit build/test/style/PR requirements and architecture targeting guidance.
- Updated `legacy/README.md` to explicitly mark compatibility paths and migration expectations.
- Hardened `scripts/run-devnet.sh` and `scripts/run-testnet.sh` with explicit RPC readiness probes before reporting startup success.

## Current canonical commands

### Build

```bash
./scripts/build.sh
```

### Lint

```bash
./scripts/lint.sh
```

### Tests

```bash
./scripts/test.sh
```

### Devnet startup

```bash
./scripts/run-devnet.sh
```

### Full audit check

```bash
./scripts/repo-audit.sh
```

### Docker devnet

```bash
docker compose up --build
```
