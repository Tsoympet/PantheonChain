#!/usr/bin/env bash
set -euo pipefail
required_docs=(
  docs/architecture.md docs/build.md docs/run-devnet.md docs/rpc.md docs/cli.md docs/tokenomics.md docs/threat-model.md docs/migration.md docs/troubleshooting.md docs/glossary.md docs/rebuild_absorption_plan.md docs/rebuild_report.md
)
required_configs=(configs/devnet/l1.json configs/devnet/l2.json configs/devnet/l3.json)
required_scripts=(scripts/build.sh scripts/test.sh scripts/lint.sh scripts/run-devnet.sh scripts/clean.sh scripts/repo-audit.sh)
required_dirs=(
  layer1-talanton/node layer1-talanton/consensus layer1-talanton/ledger layer1-talanton/tx layer1-talanton/rpc layer1-talanton/params layer1-talanton/genesis layer1-talanton/tests layer1-talanton/docs
  layer2-drachma/node layer2-drachma/consensus layer2-drachma/staking layer2-drachma/payments layer2-drachma/bridge-l1 layer2-drachma/tx layer2-drachma/rpc layer2-drachma/params layer2-drachma/genesis layer2-drachma/tests layer2-drachma/docs
  layer3-obolos/node layer3-obolos/consensus layer3-obolos/staking layer3-obolos/evm layer3-obolos/gas layer3-obolos/bridge-l2 layer3-obolos/tx layer3-obolos/rpc layer3-obolos/params layer3-obolos/genesis layer3-obolos/tests layer3-obolos/docs
  common/crypto common/p2p common/storage common/serialization common/mempool common/metrics common/rpc-common common/monetary common/utils
  relayers/relayer-l2 relayers/relayer-l3 cli/pantheon-cli configs/devnet configs/testnet configs/mainnet tests/integration tests/fixtures docker
)
for f in "${required_docs[@]}" "${required_configs[@]}"; do
  [[ -f "$f" ]] || { echo "Missing required file: $f"; exit 1; }
done
for s in "${required_scripts[@]}"; do
  [[ -x "$s" ]] || { echo "Script not executable: $s"; exit 1; }
done
for d in "${required_dirs[@]}"; do
  [[ -d "$d" ]] || { echo "Missing required directory: $d"; exit 1; }
done
for forbidden in legacy src; do
  [[ ! -d "$forbidden" ]] || { echo "Forbidden legacy directory present: $forbidden"; exit 1; }
done
for link in docs/architecture.md docs/build.md docs/run-devnet.md docs/rpc.md docs/cli.md docs/tokenomics.md docs/threat-model.md docs/migration.md docs/troubleshooting.md docs/glossary.md; do
  [[ -f "$link" ]] || { echo "README link target missing: $link"; exit 1; }
done
[[ -f docker/Dockerfile ]] || { echo "Missing docker/Dockerfile"; exit 1; }
[[ -f docker/docker-compose.yml ]] || { echo "Missing docker/docker-compose.yml"; exit 1; }
echo "Repository audit passed"
