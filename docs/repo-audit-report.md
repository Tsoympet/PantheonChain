# Repository Audit Report

## Gap scan summary

### Missing or inconsistent before this pass

- Relayer source files still lived in top-level `relayers/`, while the layered architecture defines `src/relayers/` as the canonical home.
- Core docs (`README.md`, `docs/architecture.md`, `CONTRIBUTING.md`) still referenced top-level relayer source paths.

### Added/updated in this pass

- Migrated relayer implementation files to `src/relayers/`:
  - `src/relayers/pantheon-relayer-l2.cpp`
  - `src/relayers/pantheon-relayer-l3.cpp`
- Kept `relayers/` as a compatibility build shim and documented deprecation/migration in `relayers/README.md`.
- Updated relayer references in:
  - `README.md`
  - `docs/architecture.md`
  - `CONTRIBUTING.md`
- Revalidated repo requirements with `scripts/repo-audit.sh`.

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
