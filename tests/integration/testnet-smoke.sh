#!/usr/bin/env bash
set -euo pipefail

rpc_call() {
  local port="$1"; local method="$2"; local params="${3:-[]}"
  curl -sS -X POST "http://127.0.0.1:${port}/" -H 'Content-Type: application/json' \
    --data "{\"jsonrpc\":\"2.0\",\"id\":\"1\",\"method\":\"${method}\",\"params\":${params}}"
}

for port in 28332 29332 30332; do
  curl -fsS "http://127.0.0.1:${port}/health" >/dev/null
  rpc_call "$port" chain/info '[]' | grep -q '"result"'
done

rpc_call 29332 commitments/submit '[{"source_chain":"OBOLOS","height":10}]' | grep -q 'queued'
rpc_call 29332 commitments/list '[]' | grep -q 'commitments'

echo "testnet smoke test passed"
