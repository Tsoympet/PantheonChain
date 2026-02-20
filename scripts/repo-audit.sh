#!/usr/bin/env bash
set -euo pipefail
required_docs=(
  docs/architecture.md docs/build.md docs/run-devnet.md docs/rpc.md docs/cli.md docs/tokenomics.md docs/threat-model.md docs/migration.md docs/troubleshooting.md docs/glossary.md docs/rebuild_absorption_plan.md docs/rebuild_report.md
)
required_configs=(configs/devnet/l1.json configs/devnet/l2.json configs/devnet/l3.json)
required_scripts=(scripts/build.sh scripts/test.sh scripts/lint.sh scripts/run-devnet.sh scripts/clean.sh scripts/repo-audit.sh)
for f in "${required_docs[@]}" "${required_configs[@]}"; do
  [[ -f "$f" ]] || { echo "Missing required file: $f"; exit 1; }
done
for s in "${required_scripts[@]}"; do
  [[ -x "$s" ]] || { echo "Script not executable: $s"; exit 1; }
done
for link in docs/architecture.md docs/build.md docs/run-devnet.md docs/rpc.md docs/cli.md docs/tokenomics.md docs/threat-model.md docs/migration.md docs/troubleshooting.md docs/glossary.md; do
  [[ -f "$link" ]] || { echo "README link target missing: $link"; exit 1; }
done
[[ -f docker/Dockerfile ]] || { echo "Missing docker/Dockerfile"; exit 1; }
[[ -f docker/docker-compose.yml ]] || { echo "Missing docker/docker-compose.yml"; exit 1; }
echo "Repository audit passed"
