# Rebuild Absorption Plan

## Current tree snapshot (top-level)

```text
assets/
clients/
configs/
docs/
enterprise/
include/
installers/
layer1/
layer2/
legacy/
relayers/
scripts/
src/
tests/
third_party/
tools/
```

## Classification summary

- **L1 TALANTON candidates:** `layer1/core`, `src/talanton`, `configs/*/l1.*`.
- **L2 DRACHMA candidates:** `layer2/*`, `src/drachma`, `configs/*/l2.*`.
- **L3 OBOLOS candidates:** `src/obolos`, and EVM-related modules currently in `layer1/evm` (to be relocated).
- **COMMON candidates:** `src/common`, shared headers under `include/`.
- **RELAYERS:** `src/relayers`, `relayers/`.
- **CLI:** `src/tools/pantheon_cli.cpp` and node launcher in `src/tools/pantheon_node.cpp`.
- **DOCS/CONFIGS/SCRIPTS:** `docs/`, `configs/`, `scripts/`, root manifests.

## Old → New mapping table

| old_path | new_path |
|---|---|
| `layer1/` | `layer1-talanton/` |
| `layer2/` | `layer2-drachma/` |
| `src/obolos/` | `layer3-obolos/{consensus,evm,node}` |
| `layer1/evm/` | `layer3-obolos/evm/` |
| `src/drachma/` | `layer2-drachma/{consensus,payments}/` |
| `src/talanton/` | `layer1-talanton/tx/` |
| `src/common/` | `common/` |
| `src/relayers/` + `relayers/` | `relayers/relayer-l2` and `relayers/relayer-l3` |
| `src/tools/pantheon_cli.cpp` | `cli/pantheon-cli/main.cpp` |
| `tests/integration/devnet-smoke.sh` | `tests/integration/devnet_smoke_test` |
| root `Dockerfile` + `docker-compose.yml` | `docker/Dockerfile` + `docker/docker-compose.yml` |
| `legacy/` | absorbed into canonical module paths; directory removed |

## Duplicate merge decisions

- **Consensus duplication:** keep canonical PoW validation in TALANTON modules; keep PoS consensus from `src/drachma` and `src/obolos` in their layer-specific consensus directories; remove duplicate ownership from generic `src/` tree.
- **RPC duplication:** keep layer-specific RPC handlers under each layer directory; reserve common helper utilities under `common/rpc-common`.
- **EVM duplication/conflict:** canonicalize EVM code under `layer3-obolos/evm` only; remove EVM under TALANTON.
- **CLI duplication:** canonical CLI entrypoint at `cli/pantheon-cli/main.cpp`; remove parallel source location from `src/tools`.
- **Scripts duplication:** canonical executable scripts under `scripts/` and remove stale parallel launch scripts elsewhere.

## Invariant statement

**No legacy folder will remain; all functionality preserved in canonical modules.**
