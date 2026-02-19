# Release Process

## Versioning

Use semantic versioning: `MAJOR.MINOR.PATCH`.

## Steps

1. Run `./scripts/repo-audit.sh`, `./scripts/build.sh`, `./scripts/test.sh`.
2. Bump `VERSION` and changelog/docs.
3. Create signed tag: `git tag -s vX.Y.Z -m "PantheonChain vX.Y.Z"`.
4. Push tag and publish release notes.

## Reproducibility notes

- Build with pinned toolchain in CI.
- Prefer clean checkout and deterministic config profiles.
