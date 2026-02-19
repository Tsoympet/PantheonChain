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

wait_for_rpc() {
  local url="$1"
  local name="$2"
  for _ in {1..30}; do
    if curl -sf "$url" >/dev/null; then
      return 0
    fi
    sleep 0.2
  done
  echo "ERROR: timed out waiting for ${name} RPC at ${url}" >&2
  return 1
}

wait_for_rpc "http://127.0.0.1:28332/health" "L1"
wait_for_rpc "http://127.0.0.1:29332/health" "L2"
wait_for_rpc "http://127.0.0.1:30332/health" "L3"

if [[ -x build/relayers/pantheon-relayer-l2 ]]; then
  build/relayers/pantheon-relayer-l2 >.testnet/logs/relayer-l2.log 2>&1 || true
fi
if [[ -x build/relayers/pantheon-relayer-l3 ]]; then
  build/relayers/pantheon-relayer-l3 >.testnet/logs/relayer-l3.log 2>&1 || true
fi

echo "Testnet (mock RPC harness) started on 28332/29332/30332"
echo "Stop with: xargs -r kill < <(cat .testnet/*.pid)"
