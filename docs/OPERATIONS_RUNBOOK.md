# ParthenonChain Operations Runbook

This runbook defines minimum operator procedures for lifecycle, deployment, and recovery.
It is intended for testnet/mainnet operators and SRE/security teams.

## 1. Deployment baseline

- Use only Release builds configured with real dependencies:
  - `-DPARTHENON_REQUIRE_REAL_DEPS=ON`
- Ensure the dependency gate check passes before promotion:
  - `./scripts/ci/verify_real_deps.sh`
- Pin config per-environment (`mainnet`/`testnet`/`regtest`) and forbid mixed profiles.

## 2. Startup checklist

1. Validate binary provenance/signature.
2. Validate effective chain/network config matches intended environment.
3. Start node and confirm:
   - genesis consistency checks pass,
   - chainstate rebuild completes,
   - RPC bind/auth settings are as expected.
4. Confirm health:
   - tip height advancing,
   - peer count above minimum threshold,
   - no recurring validation errors.

## 3. Shutdown and restart

- Prefer graceful stop via service manager signal and wait for clean thread teardown.
- On restart, verify no chainstate reconstruction errors were reported.
- If startup fails on persisted data inconsistency, halt and trigger incident workflow.

## 4. Backup and restore

### Backup cadence
- Block/chainstate data: periodic snapshots.
- Wallet/key material: encrypted backup with offline copy and rotation policy.
- Config/secrets: versioned secure store.

### Restore validation
1. Restore into isolated environment.
2. Start node in same network mode.
3. Verify height/hash continuity and service health.
4. Record RTO/RPO and any drift.

## 5. Security incident response

### Key compromise suspected
1. Isolate affected host.
2. Rotate credentials/tokens/certs.
3. Restore services from trusted build + verified backups.
4. Perform post-incident review and update controls.

### Dependency/supply-chain concern
1. Freeze release promotion.
2. Re-run dependency gate and provenance checks.
3. Rebuild from trusted commit and re-verify artifacts.

## 6. Release promotion gates

A release is promotable only when all are true:
- CI build + tests green.
- Dependency gate green.
- Recovery drill completed within target SLO.
- Security review sign-off complete.

## 7. Periodic drills (recommended)

- Quarterly restore drill.
- Quarterly key rotation drill.
- Semi-annual incident simulation (compromise + rollback scenario).

