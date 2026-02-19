#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

python3 scripts/validate-config.py configs/devnet/l1.json configs/devnet/l2.json configs/devnet/l3.json

mkdir -p .devnet/logs
PYTHON_BIN="/usr/bin/python3"

"$PYTHON_BIN" scripts/mock_layer_server.py 18332 l1 >.devnet/logs/l1.log 2>&1 & echo $! >.devnet/l1.pid
"$PYTHON_BIN" scripts/mock_layer_server.py 19332 l2 >.devnet/logs/l2.log 2>&1 & echo $! >.devnet/l2.pid
"$PYTHON_BIN" scripts/mock_layer_server.py 20332 l3 >.devnet/logs/l3.log 2>&1 & echo $! >.devnet/l3.pid

if [[ -x build/relayers/pantheon-relayer-l2 ]]; then
  build/relayers/pantheon-relayer-l2 >.devnet/logs/relayer-l2.log 2>&1 || true
fi
if [[ -x build/relayers/pantheon-relayer-l3 ]]; then
  build/relayers/pantheon-relayer-l3 >.devnet/logs/relayer-l3.log 2>&1 || true
fi

echo "Devnet (mock RPC harness) started on 18332/19332/20332"
echo "Stop with: xargs -r kill < <(cat .devnet/*.pid)"
