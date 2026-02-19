# Repository Audit Report

## What was missing

- No canonical layered docs set (`build.md`, `run-devnet.md`, `rpc.md`, `cli.md`, `glossary.md`).
- No standardized `configs/` network profiles for devnet/testnet/mainnet.
- Missing executable developer scripts (`build.sh`, `test.sh`, `lint.sh`, `run-devnet.sh`, `repo-audit.sh`).
- No CI workflow that validates audit + build + unit + devnet smoke end-to-end.
- No config validation utility with clear errors.
- Docker compose not aligned to L1/L2/L3 + relayers.

## What was added/updated

- Added layered docs and README overhaul.
- Added config schema + devnet profiles and validation script.
- Added script tooling for build/test/lint/devnet/audit.
- Added integration smoke test for RPC health + commitment submit/list.
- Added new CI workflow: `.github/workflows/build-test-devnet.yml`.
- Added repository hygiene files: `.editorconfig`, `.clang-format`, `CODEOWNERS`.
- Added `legacy/README.md` to explain compatibility paths.

## How to build/test/devnet now

```bash
./scripts/repo-audit.sh
./scripts/build.sh
./scripts/lint.sh
./scripts/run-devnet.sh
./scripts/test.sh
```

## One-command container startup

```bash
docker compose up --build
```
