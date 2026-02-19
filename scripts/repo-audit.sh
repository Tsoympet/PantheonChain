#!/usr/bin/env bash
set -euo pipefail

required_files=(
  README.md CONTRIBUTING.md SECURITY.md LICENSE .editorconfig .clang-format CODEOWNERS
  docs/architecture.md docs/build.md docs/run-devnet.md docs/rpc.md docs/cli.md docs/tokenomics.md docs/threat-model.md docs/migration.md docs/glossary.md
  configs/devnet/l1.json configs/devnet/l2.json configs/devnet/l3.json
  scripts/build.sh scripts/test.sh scripts/lint.sh scripts/run-devnet.sh scripts/repo-audit.sh
  .github/workflows/build-test-devnet.yml
)

for f in "${required_files[@]}"; do
  [[ -f "$f" ]] || { echo "Missing required file: $f"; exit 1; }
done

for s in scripts/build.sh scripts/test.sh scripts/lint.sh scripts/run-devnet.sh scripts/repo-audit.sh tests/integration/devnet-smoke.sh scripts/validate-config.py; do
  [[ -x "$s" ]] || { echo "Script not executable: $s"; exit 1; }
done

python3 scripts/validate-config.py configs/devnet/l1.json configs/devnet/l2.json configs/devnet/l3.json >/dev/null

rg -n "docs/(ARCHITECTURE|TOKENOMICS|NETWORKING_RPC)\.md" README.md docs || true

echo "Repository audit passed"
