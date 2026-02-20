# Build

## Canonical build method

PantheonChain uses **CMake** as the canonical build system.

```bash
./scripts/build.sh
```

This configures and builds in `build/`.

## CI-supported platforms

CI currently validates builds/tests on the following platforms:

- **Ubuntu (`ubuntu-latest`/`ubuntu-22.04`)**
  - Build, lint, repository audit
  - Unit + integration smoke coverage
  - Security/dependency checks
- **macOS (`macos-latest`)**
  - Build + test workflow coverage
- **Windows (`windows-latest`)**
  - Build + test workflow coverage

For release gating, the canonical required layered pipeline remains:

- `.github/workflows/build-test-devnet.yml`

## Manual build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j"$(nproc)"
```
