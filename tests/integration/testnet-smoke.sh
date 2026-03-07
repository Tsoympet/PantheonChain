#!/usr/bin/env bash
set -euo pipefail

rpc_call() {
  local port="$1"; local method="$2"; local params="${3:-[]}"
  curl -sS -X POST "http://127.0.0.1:${port}/" -H 'Content-Type: application/json' \
    --data "{\"jsonrpc\":\"2.0\",\"id\":\"1\",\"method\":\"${method}\",\"params\":${params}}"
}

# Basic connectivity and chain info on all three layers
for port in 28332 29332 30332; do
  curl -fsS "http://127.0.0.1:${port}/health" | grep -q '"status"'
  rpc_call "$port" chain/info '[]' | grep -q '"result"'
done

# L2 commitments (OBOLOS -> DRACHMA anchoring)
rpc_call 29332 commitments/submit '[{"source_chain":"OBOLOS","height":10}]' | grep -q 'queued'
rpc_call 29332 commitments/list '[]' | grep -q 'commitments'

# L2 transfer submit + lookup
L2_SEND=$(rpc_call 29332 transfers/send '[{"from":"alice","to":"bob","amount":"10"}]')
L2_TXID=$(echo "$L2_SEND" | python3 -c 'import json,sys; print(json.load(sys.stdin)["result"]["txid"])')
rpc_call 29332 transfers/get "[\"${L2_TXID}\"]" | grep -qE '"found"\s*:\s*true'

# L3 EVM deploy + call
L3_DEPLOY=$(rpc_call 30332 evm/deploy '[{"bytecode":"0x6001600055"}]')
L3_ADDR=$(echo "$L3_DEPLOY" | python3 -c 'import json,sys; print(json.load(sys.stdin)["result"]["address"])')
rpc_call 30332 evm/call "[{\"address\":\"${L3_ADDR}\",\"data\":\"0xabcdef01\"}]" | grep -qE '"ok"\s*:\s*true'

# Staking: stake on L2, check voting power (1A1V model)
rpc_call 29332 staking/stake '[{"address":"testaddr1","amount":500,"layer":"l2"}]' | grep -q 'accepted'
POWER=$(rpc_call 29332 staking/get_power '[{"address":"testaddr1"}]')
echo "$POWER" | grep -qE '"voting_power"\s*:\s*1'
echo "$POWER" | grep -q 'one_address_one_vote'

# Governance: submit proposal, cast vote, check proposal
GOV_SUBMIT=$(rpc_call 29332 governance/submitProposal \
  '[{"title":"Test Proposal","description":"A governance test","proposer":"testaddr1","deposit_amount":100}]')
PROP_ID=$(echo "$GOV_SUBMIT" | python3 -c 'import json,sys; print(json.load(sys.stdin)["result"]["proposal_id"])')
rpc_call 29332 governance/castVote "[{\"proposal_id\":${PROP_ID},\"voter\":\"testaddr1\",\"vote\":\"yes\"}]" | grep -q 'accepted'
rpc_call 29332 governance/getProposal "[{\"proposal_id\":${PROP_ID}}]" | grep -qE '"found"\s*:\s*true'
rpc_call 29332 governance/listProposals '[]' | grep -q 'proposals'

# Treasury balance
rpc_call 29332 treasury/getBalance '[]' | grep -q 'total'

# Ostracism status check
rpc_call 29332 ostracism/getStatus '[{"address":"testaddr1"}]' | grep -qE '"banned"\s*:\s*false'

echo "testnet smoke test passed"
