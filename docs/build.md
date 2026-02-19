# Build

## Canonical build method

PantheonChain uses **CMake**.

```bash
./scripts/build.sh
```

This configures and builds in `build/` using local dependencies or repository stubs.

## CI platform matrix

- Ubuntu (`ubuntu-latest`) for build/test/devnet smoke.
- Lint checks on Ubuntu.

## Manual build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j"$(nproc)"
```
