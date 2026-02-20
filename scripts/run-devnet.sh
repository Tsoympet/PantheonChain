#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

python3 scripts/validate-config.py configs/devnet/l1.json configs/devnet/l2.json configs/devnet/l3.json

mkdir -p .devnet/logs
PYTHON_BIN="/usr/bin/python3"

cleanup_on_error() {
  local exit_code="$?"
  if [[ "$exit_code" -eq 0 ]]; then
    return
  fi

  if compgen -G ".devnet/*.pid" >/dev/null; then
    xargs -r kill < <(cat .devnet/*.pid) || true
  fi
}
trap cleanup_on_error ERR

"$PYTHON_BIN" scripts/mock_layer_server.py 18332 l1 >.devnet/logs/l1.log 2>&1 & echo $! >.devnet/l1.pid
"$PYTHON_BIN" scripts/mock_layer_server.py 19332 l2 >.devnet/logs/l2.log 2>&1 & echo $! >.devnet/l2.pid
"$PYTHON_BIN" scripts/mock_layer_server.py 20332 l3 >.devnet/logs/l3.log 2>&1 & echo $! >.devnet/l3.pid

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

wait_for_rpc "http://127.0.0.1:18332/health" "L1"
wait_for_rpc "http://127.0.0.1:19332/health" "L2"
wait_for_rpc "http://127.0.0.1:20332/health" "L3"

trap - ERR

if [[ -x build/relayers/pantheon-relayer-l2 ]]; then
  build/relayers/pantheon-relayer-l2 >.devnet/logs/relayer-l2.log 2>&1 || true
fi
if [[ -x build/relayers/pantheon-relayer-l3 ]]; then
  build/relayers/pantheon-relayer-l3 >.devnet/logs/relayer-l3.log 2>&1 || true
fi

echo "Devnet (mock RPC harness) started on 18332/19332/20332"
echo "Stop with: xargs -r kill < <(cat .devnet/*.pid)"
