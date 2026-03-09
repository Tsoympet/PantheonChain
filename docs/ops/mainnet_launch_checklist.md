# Mainnet Launch Checklist

Use this checklist for every mainnet launch or major protocol upgrade.
Each item must be checked and signed off by at least two core team members.

---

## Phase 1 — Pre-Launch Preparation (T-60 days)

### Code & Security
- [ ] All `ctest` unit + integration tests pass on the target platform
- [ ] Full CodeQL scan completed, zero Critical/High findings
- [ ] AFL++ / libFuzzer campaign run ≥48 hours on all parser surfaces
- [ ] Independent security audit completed (firm name, report link: _____________)
- [ ] All audit findings Severity ≥ Medium resolved or accepted-with-mitigation
- [ ] `SECURITY.md` updated with audit evidence

### Configuration
- [ ] `genesis_talanton.json`, `genesis_drachma.json`, `genesis_obolos.json` finalized
- [ ] Genesis files signed by ≥3 founding validators (PGP signatures committed)
- [ ] All `configs/mainnet/*.json` `p2p_bootstrap_nodes` arrays populated with real operator IPs
- [ ] All `configs/mainnet/*.json` `dns_seeds` arrays point to live DNS servers
- [ ] `python3 scripts/validate-config.py configs/mainnet/l1.json configs/mainnet/l2.json configs/mainnet/l3.json` passes
- [ ] `python3 scripts/validate-layer-model.py` passes
- [ ] `python3 scripts/economics/check_supply_caps.py` passes
- [ ] `python3 scripts/economics/check_decimal_consistency.py` passes

### Infrastructure
- [ ] ≥8 bootstrap nodes per layer online, tested reachable from 3 independent networks
- [ ] DNS SRV/A records live for all `dns_seeds` hosts
- [ ] TLS certificates issued for all public-facing RPC endpoints
- [ ] Monitoring stack (Prometheus + Grafana) deployed and dashboards confirmed
- [ ] Alert rules enabled for: node down, sync stalled, peer count < 5, SLA breach
- [ ] Log aggregation configured (journal → ELK/Splunk/Loki)
- [ ] Backup schedule (daily snapshots, offsite replication) confirmed

### Validator Set
- [ ] Genesis validator set agreed (minimum 21 validators for BFT safety)
- [ ] Each genesis validator has generated keys using `scripts/setup-validator.sh`
- [ ] Validator keys stored on HSM or hardware wallet (never hot)
- [ ] Each genesis validator has staked the required minimum amount on testnet
- [ ] Validator set key ceremony conducted and genesis validator_set_hash committed

---

## Phase 2 — Dry Run on Testnet (T-30 days)

- [ ] Full mainnet genesis procedure rehearsed on testnet with same validator set
- [ ] All three layers started in order (L1 → L2 → L3) without manual intervention
- [ ] Cross-layer commitment anchoring verified end-to-end within 2 commitment epochs
- [ ] Relayer L2→L1 and L3→L2 confirmed operational for ≥24 hours
- [ ] At least one governance proposal created, voted on, and executed
- [ ] At least one validator jailed and unjailed (slashing rehearsal)
- [ ] Rolling upgrade rehearsal: binary upgrade on 1/3 of nodes while chain runs
- [ ] Backup and restore rehearsal: stop one node, restore from snapshot, verify sync

---

## Phase 3 — Launch Day (T-0)

### T-6h: Final checks
- [ ] All bootstrap nodes confirmed online
- [ ] DNS seeds confirmed resolving
- [ ] Final `git tag -s v2.x.x` signed and pushed
- [ ] Checksums generated and signed: `./installers/checksums/generate-checksums.sh`
- [ ] Release binaries published on GitHub Releases

### T-1h: Validator preparation
- [ ] All genesis validators confirm keys loaded and signing ready
- [ ] All validators confirm network time synchronized (NTP delta < 1s)
- [ ] Communication channel (Telegram/Discord/Matrix) confirmed active

### T-0: Genesis
- [ ] L1 TALANTON node started by agreed genesis block producer
- [ ] L2 DRACHMA validators confirm first epoch sealed
- [ ] L3 OBOLOS validators confirm first epoch sealed
- [ ] First commitment from L3→L2 confirmed on-chain
- [ ] First commitment from L2→L1 confirmed on-chain
- [ ] Block explorer shows all three layers producing blocks
- [ ] RPC health checks passing: `GET /health` on all three layers returns `"status":"ok"`
- [ ] Prometheus metrics visible at `/metrics` on all three layers

### T+1h: Stability confirmation
- [ ] No unplanned hard forks observed
- [ ] Block times within expected range (L1: ~10min, L2/L3: ~6s)
- [ ] No validator slashing events
- [ ] No commitment relay failures
- [ ] Incident channel has no active P0/P1 alerts

---

## Phase 4 — Post-Launch (T+7 days)

- [ ] Public announcement published
- [ ] Block explorer publicly accessible
- [ ] Documentation updated with mainnet endpoint URLs
- [ ] Testnet faucet disabled (to prevent token confusion)
- [ ] Bug bounty program activated
- [ ] Retrospective completed and lessons-learned documented

---

## Emergency Rollback Procedure

If a critical issue is discovered within the first 2 hours:

1. Core team votes (2/3 majority) to pause the network.
2. Validators coordinate a halt by refusing to sign new blocks.
3. A patch is prepared, tested, and committed.
4. Validators agree on a restart block height.
5. All validators upgrade and restart at the agreed height.
6. Post-mortem written and published within 72 hours.

---

*Last updated: 2026-03-09*
*Owner: PantheonChain Core Team*
