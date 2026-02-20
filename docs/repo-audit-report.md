# Repository Audit Report

## Gap scan summary

### Missing or inconsistent before this pass

- `scripts/repo-audit.sh` validated structure/files/configs, but did not verify:
  - that key CI jobs/tokens remain present in the canonical devnet workflow;
  - that required docs exposed by README/operations docs keep valid local markdown links.
- `docs/DPDK_NETWORKING.md` contained a stale local link (`./NETWORK_ARCHITECTURE.md`) to a non-existent file.

### Added/updated in this pass

- Extended `scripts/repo-audit.sh` to include:
  - mandatory presence checks for `.github/workflows/build.yml` and `.github/workflows/test.yml`;
  - CI token checks in `.github/workflows/build-test-devnet.yml` (`build`, `unit-tests`, `integration-devnet-smoke`, cache, artifact upload);
  - local markdown link validation for canonical root/docs contributor and operator docs.
- Fixed stale docs link in `docs/DPDK_NETWORKING.md`:
  - `./NETWORK_ARCHITECTURE.md` -> `./architecture.md`.
- Re-ran lint, audit, and test/devnet smoke commands to confirm repository coherence.

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
