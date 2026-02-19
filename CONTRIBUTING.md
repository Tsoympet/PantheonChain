# Contributing to PantheonChain

Thanks for contributing to PantheonChain.

## Development workflow

1. Create a feature branch from `main`.
2. Run repository hygiene checks and local validation:
   - `./scripts/repo-audit.sh`
   - `./scripts/build.sh`
   - `./scripts/lint.sh`
   - `./scripts/test.sh`
3. Update docs/configs whenever behavior, interfaces, or operations change.
4. Ensure all scripts remain executable (`chmod +x scripts/*.sh tests/integration/*.sh`).
5. Open a pull request with a complete summary and test evidence.

## Canonical project layout

Contributions should target the layered architecture:

- `src/common` (shared primitives)
- `src/talanton` (L1)
- `src/drachma` (L2)
- `src/obolos` (L3)
- `src/relayers` and `relayers` (relay services)
- `src/tools` (CLI/node tool integrations)
- `configs/{devnet,testnet,mainnet}` (network profiles)
- `tests/{unit,integration,fixtures}`

Legacy compatibility folders are documented in `legacy/README.md` and should not receive new feature work unless migration-specific.

## Style and formatting

- C++ style is governed by `.clang-format`.
- Editor normalization is governed by `.editorconfig`.
- Shell scripts should be POSIX/Bash compatible and pass `bash -n`.
- Keep commits focused and avoid unrelated refactors.

## Pull request rules

Every PR must include:

- Problem statement and implementation summary.
- Risk/rollback notes for operational or consensus changes.
- Exact commands run locally and their results.
- Documentation updates for any user/operator-visible change.

