# PantheonChain – Security Incident Response Runbook

---

## 1. Incident Classification

| Severity | Examples | Max time-to-containment |
|----------|---------|------------------------|
| P0 – Critical | Chain halt, double-spend, fund theft, consensus split | 15 min |
| P1 – High | Governance attack, Sybil node flood, eclipse attack | 1 hour |
| P2 – Medium | RPC outage, mempool spam, single validator equivocation | 4 hours |
| P3 – Low | Slow sync, peer churn, non-critical alert | Next business day |

---

## 2. First 5 Minutes – Triage

```bash
# Check node health
pantheon-cli getinfo

# Check for chain split (compare tip hash with a trusted peer)
MYTIP=$(pantheon-cli getblockhash $(pantheon-cli getblockcount))
echo "My tip: ${MYTIP}"
# Compare with https://explorer.pantheonchain.io or a trusted peer.

# Check mempool for anomalies (huge volume, spam)
pantheon-cli getmempoolinfo

# Check if governance has been tampered with
pantheon-cli governance list_proposals
```

If the chain is halted or a split is detected → escalate to P0 immediately.

---

## 3. P0 – Chain Halt or Consensus Failure

### 3.1 Containment

```bash
# Stop the node to prevent further divergence.
sudo systemctl stop pantheonchain

# Collect logs before any state changes.
sudo journalctl -u pantheonchain --since "1 hour ago" > /tmp/pantheonchain-p0-$(date +%Y%m%dT%H%M%SZ).log
sudo cp -r /var/lib/pantheonchain /tmp/pantheonchain-snapshot-$(date +%Y%m%dT%H%M%SZ)
```

### 3.2 Coordinate with Other Validators

1. Contact other validators via the emergency out-of-band channel (Signal group / PGP email).
2. Agree on a canonical chain tip (use block height + hash).
3. If ≥ 2/3 of validators agree on the canonical tip, restart from that checkpoint.

### 3.3 Recovery

```bash
# Roll back to the last known-good checkpoint.
sudo systemctl stop pantheonchain
sudo rm -rf /var/lib/pantheonchain/chainstate
# Restore from the most recent backup that is on the canonical chain:
# See backup-restore.md § 6.2

# Restart.
sudo systemctl start pantheonchain

# Verify the node is on the canonical chain.
MYTIP=$(pantheon-cli getblockhash $(pantheon-cli getblockcount))
echo "Tip after recovery: ${MYTIP}"
```

---

## 4. P1 – Governance Attack

Signs of a governance attack:
- A proposal passed with abnormally high voting power from a single address.
- A CONSTITUTIONAL proposal raised without the required 2/3 threshold.
- A TREASURY_SPENDING proposal withdrawn immediately after execution.

### 4.1 Pause Governance (Emergency Council)

```bash
# If the Emergency Council quorum is available, pause governance:
pantheon-cli emergency submit \
  --action PAUSE_GOVERNANCE \
  --guardians <GUARDIAN_1,GUARDIAN_2,...>
```

### 4.2 Veto the Malicious Proposal

```bash
# Cast VETO votes from all available validator accounts.
pantheon-cli governance vote \
  --proposal_id <PROPOSAL_ID> \
  --voter <VALIDATOR_ADDRESS> \
  --choice VETO \
  --signature <SIG>
```

A VETO rate > 33.34 % of total voting power will reject the proposal and slash the proposer's
deposit (Cosmos Hub model).

### 4.3 Submit a Corrective Emergency Proposal

```bash
pantheon-cli governance submit_proposal \
  --type EMERGENCY \
  --title "Reverse malicious treasury spend <PROPOSAL_ID>" \
  --proposer <TRUSTED_ADDRESS> \
  --execution_data <REVERSAL_EXECUTION_DATA>
```

---

## 5. P2 – RPC Outage

```bash
# Check the RPC server process.
sudo systemctl status pantheonchain

# Check if the RPC port is listening.
ss -tlnp | grep 8332

# Restart the RPC server (will restart the full node).
sudo systemctl restart pantheonchain

# Check for rate-limiter misconfiguration.
grep -i rate /etc/pantheonchain/pantheonchain.conf
```

---

## 6. Post-Incident Checklist

Complete the following within 48 hours of any P0 or P1 incident:

- [ ] Incident timeline documented (what happened, when, who responded).
- [ ] Root cause identified (software bug, operator error, external attack, key compromise).
- [ ] Impacted users notified (if funds or governance rights were affected).
- [ ] Corrective action implemented and verified.
- [ ] Monitoring / alerting improved to detect recurrence earlier.
- [ ] If a key was compromised: follow `key-compromise.md`.
- [ ] If a bug was found: file a GitHub issue or security advisory.
- [ ] Post-mortem report filed in the incident log (see internal Confluence / Notion).

---

## 7. Incident Log Format

Create a file `incidents/YYYY-MM-DD-<slug>.md` with:

```markdown
# Incident: <short description>

Date: YYYY-MM-DD HH:MM UTC
Severity: P0 / P1 / P2 / P3
Status: Open / Resolved
Reporter: <name>

## Timeline
- HH:MM – <event>
- HH:MM – <event>

## Root Cause
<description>

## Remediation
<description>

## Follow-up Actions
- [ ] Action 1 (owner, due date)
- [ ] Action 2 (owner, due date)
```

---

## 8. Useful Commands Reference

```bash
# Node status
pantheon-cli getinfo

# Chain tip
pantheon-cli getblockhash $(pantheon-cli getblockcount)

# Active governance proposals
pantheon-cli governance list_proposals

# Current staking totals
pantheon-cli staking get_power --address <ADDRESS>

# Treasury balances
pantheon-cli treasury balance

# Active ostracism bans
pantheon-cli ostracism list_bans --block_height $(pantheon-cli getblockcount)

# Emergency council pause governance
pantheon-cli emergency submit --action PAUSE_GOVERNANCE
```
