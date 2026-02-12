#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT_DIR"

check_file() {
  local path="$1"
  if [[ -f "$path" ]]; then
    echo "[OK] file exists: $path"
  else
    echo "[FAIL] missing file: $path" >&2
    return 1
  fi
}

check_symbol() {
  local pattern="$1"
  local path="$2"
  if rg -n "$pattern" "$path" > /dev/null; then
    echo "[OK] symbol '$pattern' found in $path"
  else
    echo "[FAIL] symbol '$pattern' missing from $path" >&2
    return 1
  fi
}

echo "== Verifying advanced feature implementations =="

# Performance optimizations
check_file "layer1/core/sharding/shard.h"
check_symbol "struct ShardConfig" "layer1/core/sharding/shard.h"
check_symbol "class ShardStateManager" "layer1/core/sharding/shard.h"
check_symbol "class ShardCoordinator" "layer1/core/sharding/shard.h"

check_file "layer2/plasma/plasma_chain.h"
check_symbol "class PlasmaChain" "layer2/plasma/plasma_chain.h"
check_symbol "RequestExit" "layer2/plasma/plasma_chain.h"
check_symbol "VerifyMerkleProof" "layer2/plasma/plasma_chain.h"

check_file "layer2/rollups/optimistic_rollup.h"
check_symbol "class OptimisticRollup" "layer2/rollups/optimistic_rollup.h"
check_symbol "CreateBatch" "layer2/rollups/optimistic_rollup.h"
check_symbol "SubmitBatch" "layer2/rollups/optimistic_rollup.h"

# Privacy features
check_file "layer1/core/privacy/zk_snark.h"
check_symbol "class ZKProver" "layer1/core/privacy/zk_snark.h"
check_symbol "class PedersenCommitment" "layer1/core/privacy/zk_snark.h"
check_symbol "class Nullifier" "layer1/core/privacy/zk_snark.h"

check_file "layer1/core/privacy/ring_signature.h"
check_symbol "class RingSigner" "layer1/core/privacy/ring_signature.h"
check_symbol "class RingVerifier" "layer1/core/privacy/ring_signature.h"
check_symbol "class StealthAddress" "layer1/core/privacy/ring_signature.h"

# Governance + contracts
check_file "layer1/governance/voting.h"
check_symbol "class VotingSystem" "layer1/governance/voting.h"
check_symbol "CreateProposal" "layer1/governance/voting.h"
check_symbol "CastVote" "layer1/governance/voting.h"

check_file "layer1/evm/formal_verification/verifier.h"
check_symbol "class ContractVerifier" "layer1/evm/formal_verification/verifier.h"
check_symbol "GetStandardProperties" "layer1/evm/formal_verification/verifier.h"
check_symbol "class UpgradeableContract" "layer1/evm/formal_verification/verifier.h"

# Developer tools
check_file "tools/block_explorer/explorer.h"
check_file "tools/mobile_sdks/README.md"
check_file "tools/ide_plugins/README.md"
check_file "tools/debugging/tracer.h"
check_symbol "class TransactionTracer" "tools/debugging/tracer.h"
check_symbol "class Profiler" "tools/debugging/tracer.h"
check_file "tools/testing/framework.h"
check_symbol "class TestSuite" "tools/testing/framework.h"

# Enterprise
check_file "enterprise/permissioned.h"
check_symbol "class PermissionedMode" "enterprise/permissioned.h"
check_symbol "class ConsortiumManager" "enterprise/permissioned.h"
check_symbol "class ComplianceManager" "enterprise/permissioned.h"
check_symbol "class AuditLogger" "enterprise/permissioned.h"
check_symbol "class SLAMonitor" "enterprise/permissioned.h"

echo "== All advanced feature checks passed =="
