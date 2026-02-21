#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
python3 scripts/validate-config.py configs/devnet/l1.json configs/devnet/l2.json configs/devnet/l3.json
mkdir -p .devnet/logs .devnet/state

python3 scripts/devnet_layer_server.py 18332 l1 .devnet/state/l1.json >.devnet/logs/l1.log 2>&1 & echo $! >.devnet/l1.pid
python3 scripts/devnet_layer_server.py 19332 l2 .devnet/state/l2.json >.devnet/logs/l2.log 2>&1 & echo $! >.devnet/l2.pid
python3 scripts/devnet_layer_server.py 20332 l3 .devnet/state/l3.json >.devnet/logs/l3.log 2>&1 & echo $! >.devnet/l3.pid

rpc_call() {
  local port="$1"; local method="$2"; local params="${3:-[]}"
  curl -sS -X POST "http://127.0.0.1:${port}/" -H 'Content-Type: application/json' \
    --data "{\"jsonrpc\":\"2.0\",\"id\":\"1\",\"method\":\"${method}\",\"params\":${params}}"
}

for p in 18332 19332 20332; do
  for _ in {1..50}; do curl -sf "http://127.0.0.1:${p}/health" >/dev/null && break; sleep 0.2; done
done

# Seed deterministic anchoring chain OBOLOS -> DRACHMA -> TALANTON
OB_HASH="0x0b010203"
DR_HASH="0x0d040506"
rpc_call 19332 commitments/submit "[{\"id\":\"l2-anchor-1\",\"source_chain\":\"OBOLOS\",\"obolos_hash\":\"${OB_HASH}\",\"height\":12}]" >/dev/null
rpc_call 18332 commitments/submit "[{\"id\":\"l1-anchor-1\",\"source_chain\":\"DRACHMA\",\"drachma_hash\":\"${DR_HASH}\",\"references\":[\"${OB_HASH}\"],\"height\":33}]" >/dev/null

echo "Devnet started"
