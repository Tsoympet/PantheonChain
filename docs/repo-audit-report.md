# Repository Audit Report

## Gap scan summary

### Missing or inconsistent before this pass

- Multiple GitHub workflows pinned to non-existent `@v6` major versions (`checkout`, `upload-artifact`, `setup-node`, `setup-python`), which would fail in CI.
- Audit script checks were too narrow (did not enforce supported GitHub Action major versions).
- Build documentation did not clearly enumerate all platforms currently covered by CI workflows.

### Added/updated in this pass

- Updated workflows under `.github/workflows/` to supported major versions:
  - `actions/checkout@v4`
  - `actions/upload-artifact@v4`
  - `actions/setup-node@v4`
  - `actions/setup-python@v5`
- Extended `scripts/repo-audit.sh` to fail when unsupported `@v6` action pins are present.
- Updated `docs/build.md` with explicit CI-supported platform coverage (Ubuntu, macOS, Windows) and canonical release gate workflow.

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
