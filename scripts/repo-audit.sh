#!/usr/bin/env bash
set -euo pipefail

required_dirs=(
  src/common src/talanton src/drachma src/obolos src/relayers src/tools
  docs scripts configs tests
  tests/unit tests/integration tests/fixtures
  configs/devnet configs/testnet configs/mainnet
)

required_files=(
  README.md CONTRIBUTING.md SECURITY.md LICENSE .editorconfig .clang-format CODEOWNERS Dockerfile docker-compose.yml
  docs/architecture.md docs/build.md docs/run-devnet.md docs/rpc.md docs/cli.md docs/tokenomics.md docs/threat-model.md docs/migration.md docs/glossary.md docs/repo-audit-report.md docs/release.md docs/operations.md
  configs/config.schema.json
  configs/devnet/l1.json configs/devnet/l2.json configs/devnet/l3.json
  configs/testnet/l1.json configs/testnet/l2.json configs/testnet/l3.json
  configs/mainnet/l1.json configs/mainnet/l2.json configs/mainnet/l3.json
  scripts/build.sh scripts/test.sh scripts/lint.sh scripts/run-devnet.sh scripts/repo-audit.sh scripts/validate-config.py
  tests/integration/devnet-smoke.sh
  .github/workflows/build-test-devnet.yml
)

for d in "${required_dirs[@]}"; do
  [[ -d "$d" ]] || { echo "Missing required directory: $d"; exit 1; }
done

for f in "${required_files[@]}"; do
  [[ -f "$f" ]] || { echo "Missing required file: $f"; exit 1; }
done

for s in scripts/build.sh scripts/test.sh scripts/lint.sh scripts/run-devnet.sh scripts/repo-audit.sh tests/integration/devnet-smoke.sh scripts/validate-config.py; do
  [[ -x "$s" ]] || { echo "Script not executable: $s"; exit 1; }
done

python3 scripts/validate-config.py \
  configs/devnet/l1.json configs/devnet/l2.json configs/devnet/l3.json \
  configs/testnet/l1.json configs/testnet/l2.json configs/testnet/l3.json \
  configs/mainnet/l1.json configs/mainnet/l2.json configs/mainnet/l3.json >/dev/null

# Ensure docs avoid stale uppercase links and old networking doc aliases.
if rg -n "docs/(ARCHITECTURE|TOKENOMICS|NETWORKING_RPC)\.md" README.md docs >/dev/null; then
  echo "Found stale uppercase/legacy docs references"
  rg -n "docs/(ARCHITECTURE|TOKENOMICS|NETWORKING_RPC)\.md" README.md docs
  exit 1
fi

# Ensure workflows use currently supported major action versions.
if rg -n "actions/(checkout|upload-artifact|setup-node|setup-python)@v6" .github/workflows >/dev/null; then
  echo "Found unsupported GitHub Action major versions (@v6)"
  rg -n "actions/(checkout|upload-artifact|setup-node|setup-python)@v6" .github/workflows
  exit 1
fi

echo "Repository audit passed"
