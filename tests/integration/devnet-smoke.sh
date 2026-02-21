#!/usr/bin/env bash
set -euo pipefail

rpc_call() {
  local port="$1"; local method="$2"; local params="${3:-[]}"
  curl -sS -X POST "http://127.0.0.1:${port}/" -H 'Content-Type: application/json' \
    --data "{\"jsonrpc\":\"2.0\",\"id\":\"1\",\"method\":\"${method}\",\"params\":${params}}"
}

for port in 18332 19332 20332; do
  curl -fsS "http://127.0.0.1:${port}/health" | grep -qE '"status"\s*:\s*"ok"'
  curl -fsS "http://127.0.0.1:${port}/chain/info" | grep -q '"layer"'
  rpc_call "$port" chain/info '[]' | grep -q '"result"'
done

# L2 transfer send + lookup
L2_SEND=$(rpc_call 19332 transfers/send '[{"from":"alice","to":"bob","amount":"10"}]')
L2_TXID=$(echo "$L2_SEND" | python3 -c 'import json,sys; print(json.load(sys.stdin)["result"]["txid"])')
rpc_call 19332 transfers/get "[\"${L2_TXID}\"]" | grep -qE '"found"\s*:\s*true'

# L3 deploy + call
L3_DEPLOY=$(rpc_call 20332 evm/deploy '[{"bytecode":"0x6001600055"}]')
L3_ADDR=$(echo "$L3_DEPLOY" | python3 -c 'import json,sys; print(json.load(sys.stdin)["result"]["address"])')
rpc_call 20332 evm/call "[{\"address\":\"${L3_ADDR}\",\"data\":\"0xabcdef01\"}]" | grep -qE '"ok"\s*:\s*true'

# Anchoring verification OBOLOS -> DRACHMA -> TALANTON
L2_COMMIT=$(rpc_call 19332 commitments/get '["l2-anchor-1"]')
echo "$L2_COMMIT" | grep -qE '"source_chain"\s*:\s*"OBOLOS"'
OB_HASH=$(echo "$L2_COMMIT" | python3 -c 'import json,sys; print(json.load(sys.stdin)["result"]["commitment"]["obolos_hash"])')

L1_COMMIT=$(rpc_call 18332 commitments/get '["l1-anchor-1"]')
echo "$L1_COMMIT" | grep -qE '"source_chain"\s*:\s*"DRACHMA"'
echo "$L1_COMMIT" | grep -q "$OB_HASH"

echo "devnet smoke test passed"
