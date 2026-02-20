#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
python3 scripts/validate-config.py configs/devnet/l1.json configs/devnet/l2.json configs/devnet/l3.json
mkdir -p .devnet/logs
python3 scripts/mock_layer_server.py 18332 l1 >.devnet/logs/l1.log 2>&1 & echo $! >.devnet/l1.pid
python3 scripts/mock_layer_server.py 19332 l2 >.devnet/logs/l2.log 2>&1 & echo $! >.devnet/l2.pid
python3 scripts/mock_layer_server.py 20332 l3 >.devnet/logs/l3.log 2>&1 & echo $! >.devnet/l3.pid
for p in 18332 19332 20332; do
  for _ in {1..30}; do curl -sf "http://127.0.0.1:${p}/health" >/dev/null && break; sleep 0.2; done
done
[ -x build/pantheon-relayer-l3 ] && build/pantheon-relayer-l3 >.devnet/logs/relayer-l3.log 2>&1 || true
[ -x build/pantheon-relayer-l2 ] && build/pantheon-relayer-l2 >.devnet/logs/relayer-l2.log 2>&1 || true
echo "Devnet started"
