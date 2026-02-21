# Testnet Ready Report

## What Changed

- Added deterministic CMake presets (`dev`, `release`, `ci`) in `CMakePresets.json`.
- Added placeholder gate script and CI enforcement.
- Reworked devnet launcher and integration smoke to validate RPC health, L2 transfer flow, L3 EVM deploy/call flow, and commitment anchoring chain OBOLOS → DRACHMA → TALANTON.
- Added operator runbooks and backup/restore scripts for node data.
- Added `docs/testnet_definition_of_done.md` as a concrete pass/fail checklist.

## Build and Run

```bash
./scripts/build.sh
./scripts/run-devnet.sh
./tests/integration/devnet-smoke.sh
```

## Smoke Test

```bash
./scripts/test.sh
```

## Known Limitations

- Full fraud proofs / validity proofs with challenge games are out of scope and not implemented.
