# Contributing to PantheonChain

## Development flow

1. Create a feature branch.
2. Run:
   - `./scripts/build.sh`
   - `./scripts/lint.sh`
   - `./scripts/test.sh`
3. Update docs/configs for any operator-visible behavior.
4. Open a PR with testing evidence.

## Style rules

- C++ formatting: `.clang-format`
- Editor defaults: `.editorconfig`
- Keep dependencies minimal.
- Do not modify consensus logic unless explicitly scoped.

## PR rules

- Include summary + risk assessment.
- Include commands run and results.
- Keep commits focused and reviewable.
