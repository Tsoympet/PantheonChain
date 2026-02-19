# Build

## Canonical build method

PantheonChain uses **CMake** as the canonical build system.

```bash
./scripts/build.sh
```

This configures and builds in `build/`.

## CI-supported platforms

Current required CI pipeline (`.github/workflows/build-test-devnet.yml`) runs on:

- `ubuntu-latest` for:
  - repository audit
  - build
  - unit tests
  - integration devnet smoke

Additional legacy workflows may validate other platforms, but `ubuntu-latest` is the canonical required gate.

## Manual build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j"$(nproc)"
```
