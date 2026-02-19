#!/usr/bin/env bash
set -euo pipefail

rpc_call() {
  local port="$1"; local method="$2"; local params="${3:-[]}"
  curl -sS -X POST "http://127.0.0.1:${port}/" -H 'Content-Type: application/json' \
    --data "{\"jsonrpc\":\"2.0\",\"id\":\"1\",\"method\":\"${method}\",\"params\":${params}}"
}

for port in 18332 19332 20332; do
  curl -fsS "http://127.0.0.1:${port}/health" >/dev/null
  rpc_call "$port" chain/info '[]' | rg -q '"result"'
done

rpc_call 19332 commitments/submit '[{"source_chain":"OBOLOS","height":10}]' | rg -q 'queued'
rpc_call 19332 commitments/list '[]' | rg -q 'commitments'

echo "devnet smoke test passed"
