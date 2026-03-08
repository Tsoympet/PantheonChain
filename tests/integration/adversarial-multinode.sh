#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

mkdir -p .multinode/logs .multinode/state

python3 scripts/devnet_layer_server.py 29332 l2 .multinode/state/l2a.json >.multinode/logs/l2a.log 2>&1 & echo $! >.multinode/l2a.pid
python3 scripts/devnet_layer_server.py 29333 l2 .multinode/state/l2b.json >.multinode/logs/l2b.log 2>&1 & echo $! >.multinode/l2b.pid
python3 scripts/devnet_layer_server.py 30332 l3 .multinode/state/l3a.json >.multinode/logs/l3a.log 2>&1 & echo $! >.multinode/l3a.pid
python3 scripts/devnet_layer_server.py 30333 l3 .multinode/state/l3b.json >.multinode/logs/l3b.log 2>&1 & echo $! >.multinode/l3b.pid

cleanup() {
  xargs -r kill < <(cat .multinode/*.pid) || true
}
trap cleanup EXIT

for p in 29332 29333 30332 30333; do
  for _ in {1..50}; do
    curl -sf "http://127.0.0.1:${p}/health" >/dev/null && break
    sleep 0.2
  done
done

rpc() {
  local port="$1"; local payload="$2"
  curl -sS -X POST "http://127.0.0.1:${port}/" -H 'Content-Type: application/json' --data "$payload" >/dev/null
}

# Equivocation scenario: same commitment id, conflicting payload across peers.
rpc 29332 '{"jsonrpc":"2.0","id":"1","method":"commitments/submit","params":[{"id":"equiv-1","source_chain":"OBOLOS","obolos_hash":"0xaaa","height":99}]}'
rpc 29333 '{"jsonrpc":"2.0","id":"1","method":"commitments/submit","params":[{"id":"equiv-1","source_chain":"OBOLOS","obolos_hash":"0xbbb","height":99}]}'

# Liveness/churn scenario: drop one node.
kill "$(cat .multinode/l3b.pid)" || true
sleep 0.5
rm -f .multinode/l3b.pid

if python3 scripts/runtime/multinode_adversarial_check.py \
  --states .multinode/state/l2a.json .multinode/state/l2b.json .multinode/state/l3a.json .multinode/state/l3b.json \
  --min-live 4; then
  echo "Expected adversarial checker to fail, but it passed" >&2
  exit 1
fi

echo "Adversarial multi-node scenario correctly detected" 
