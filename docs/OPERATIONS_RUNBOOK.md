# ParthenonChain Operations Runbook

This runbook defines minimum operator procedures for lifecycle, deployment, and recovery.
It is intended for testnet/mainnet operators and SRE/security teams.

## 1. Deployment baseline

- Use only Release builds configured with real dependencies:
  - `-DPARTHENON_REQUIRE_REAL_DEPS=ON`
- Ensure the dependency gate check passes before promotion:
  - `./scripts/ci/verify_real_deps.sh`
- Pin config per-environment (`mainnet`/`testnet`/`regtest`) and forbid mixed profiles.
- Generate and archive build provenance (commit hash, build host, toolchain version, SBOM checksum).
- Verify binary signatures and published hashes before deployment to each environment.

### Deployment workflow

1. Stage binaries in a quarantine bucket.
2. Run smoke tests against a regtest node (RPC health, basic transaction, chainstate rebuild).
3. Promote to testnet canary nodes and monitor for 1-2 hours.
4. Roll out to production in waves (10/50/100%) with health checks at each stage.
5. Record release metadata (git SHA, config checksum, rollout time).

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
- For planned maintenance, stop miners first, then validators, then RPC edge nodes.
- Capture final block height and tip hash before shutdown for post-restart verification.

## 4. Backup and restore

### Backup cadence
- Block/chainstate data: periodic snapshots.
- Wallet/key material: encrypted backup with offline copy and rotation policy.
- Config/secrets: versioned secure store.
- Snapshot metadata must include chain height, tip hash, and snapshot creation timestamp.

### Backup runbook
1. Pause miner/validator writes (or set node to read-only mode).
2. Snapshot `chainstate/`, `blocks/`, and wallet databases.
3. Encrypt and store snapshots in two geographically separated locations.
4. Validate checksums before unlocking services.

### Restore validation
1. Restore into isolated environment.
2. Start node in same network mode.
3. Verify height/hash continuity and service health.
4. Record RTO/RPO and any drift.
5. Promote restored node only after full RPC + consensus checks pass.

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
4. Revoke affected keys/certs and rotate publishing credentials.

### Release incident (consensus, rollback, or exploit)
1. Halt rollout and freeze new transactions (if possible).
2. Identify affected release/feature flag and revert to last known good build.
3. Coordinate with partner operators on rollback height and chainstate snapshot.
4. Publish advisory with remediation steps and monitoring guidance.

## 6. Release promotion gates

A release is promotable only when all are true:
- CI build + tests green.
- Dependency gate green.
- Recovery drill completed within target SLO.
- Security review sign-off complete.
- Regtest smoke test suite green.

## 7. Periodic drills (recommended)

- Quarterly restore drill.
- Quarterly key rotation drill.
- Semi-annual incident simulation (compromise + rollback scenario).
- Annual disaster recovery failover exercise (multi-region switch).

## 8. Recovery and rollback checklist

1. Identify last known good block height and hash.
2. Restore node snapshots to staging and re-run chainstate rebuild.
3. Confirm mempool replay and wallet reconciliation succeed.
4. Deploy fixed build with feature flags disabled for impacted components.
5. Re-enable traffic gradually and monitor consensus health metrics.
