#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

python3 scripts/validate-config.py configs/testnet/l1.json configs/testnet/l2.json configs/testnet/l3.json

mkdir -p .testnet/logs
PYTHON_BIN="/usr/bin/python3"

"$PYTHON_BIN" scripts/mock_layer_server.py 28332 l1 >.testnet/logs/l1.log 2>&1 & echo $! >.testnet/l1.pid
"$PYTHON_BIN" scripts/mock_layer_server.py 29332 l2 >.testnet/logs/l2.log 2>&1 & echo $! >.testnet/l2.pid
"$PYTHON_BIN" scripts/mock_layer_server.py 30332 l3 >.testnet/logs/l3.log 2>&1 & echo $! >.testnet/l3.pid

if [[ -x build/relayers/pantheon-relayer-l2 ]]; then
  build/relayers/pantheon-relayer-l2 >.testnet/logs/relayer-l2.log 2>&1 || true
fi
if [[ -x build/relayers/pantheon-relayer-l3 ]]; then
  build/relayers/pantheon-relayer-l3 >.testnet/logs/relayer-l3.log 2>&1 || true
fi

echo "Testnet (mock RPC harness) started on 28332/29332/30332"
echo "Stop with: xargs -r kill < <(cat .testnet/*.pid)"
