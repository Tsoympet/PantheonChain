# Validator Operations Runbook

This runbook covers the complete lifecycle of a PantheonChain validator
operating on L2 (DRACHMA) and/or L3 (OBOLOS).

---

## 1. Initial Setup

### 1.1 System Requirements

| Resource | Minimum | Recommended |
|----------|---------|-------------|
| CPU      | 4 cores | 8+ cores    |
| RAM      | 8 GB    | 16–32 GB    |
| Disk     | 500 GB SSD | 2 TB NVMe |
| Network  | 100 Mbps | 1 Gbps     |
| OS       | Ubuntu 22.04 LTS | Ubuntu 22.04 LTS |

### 1.2 Install the Node Binary

```bash
# Option A: from DEB package
wget https://releases.pantheonchain.org/pantheonchain_2.3.0_amd64.deb
sha256sum -c SHA256SUMS
sudo dpkg -i pantheonchain_2.3.0_amd64.deb
sudo systemctl enable --now pantheon-l2

# Option B: build from source
git clone https://github.com/Tsoympet/PantheonChain.git
cd PantheonChain
./scripts/build.sh
sudo cmake --install build
```

### 1.3 Generate Validator Keys

Use the interactive setup wizard:

```bash
./scripts/setup-validator.sh --layer=l2
```

The wizard will:
1. Generate a secp256k1 keypair (BIP-340 Schnorr)
2. Derive the validator address
3. Display the public key for registration
4. Prompt you to store the private key securely (hardware wallet recommended)

**Security**: Never store the private key in plain text on disk.
Use a hardware wallet (Ledger/Trezor) or HSM in production.

### 1.4 Register Validator

```bash
pantheon-cli validator keys-import \
    --layer=l2 \
    --pubkey=<your_validator_pubkey_hex>

# Verify registration
pantheon-cli validator status --layer=l2 --json
```

### 1.5 Stake Tokens

```bash
pantheon-cli staking deposit \
    --layer=l2 \
    --amount=<amount_in_base_units> \
    --address=<your_validator_address>

# Check staking status
pantheon-cli staking status --layer=l2 --address=<your_address>
```

Minimum stake: see `genesis_drachma.json` → `minimum_stake`.

---

## 2. Running the Validator

### 2.1 Start with systemd

```bash
sudo systemctl start pantheon-l2
sudo systemctl status pantheon-l2
journalctl -fu pantheon-l2
```

### 2.2 Verify Participation

A healthy validator should:
- Be included in the active validator set each epoch
- Appear in commitment signatures
- Not appear in slashing events

```bash
# Check sync status
pantheon-cli node sync-status --layer=l2

# Check peer connectivity
pantheon-cli node peer-info --layer=l2

# Check staking power
pantheon-cli staking status --layer=l2 --address=<your_address>
```

### 2.3 Prometheus Metrics

The node exposes Prometheus metrics at `http://127.0.0.1:9332/metrics`.
Key metrics to monitor:

| Metric | Alert Threshold |
|--------|----------------|
| `pantheon_best_block_height` | Must increase; alert if stalled >5 min |
| `pantheon_peer_count` | Alert if < 5 |
| `pantheon_syncing` | Alert if 1 for >30 min |
| `pantheon_rpc_requests_total{status="error"}` | Alert if error rate > 1% |
| `pantheon_rpc_request_duration_seconds_sum` | Alert if p99 > 5s |

---

## 3. Maintenance Procedures

### 3.1 Graceful Restart (Rolling Upgrade)

To upgrade without missing block proposals:
1. Confirm you are not the next scheduled proposer (check logs).
2. Stop the node: `sudo systemctl stop pantheon-l2`
3. Install new binary.
4. Start: `sudo systemctl start pantheon-l2`
5. Verify sync: `pantheon-cli node sync-status --layer=l2`
6. Confirm rejoined validator set before next epoch.

### 3.2 Key Rotation

To rotate your validator signing key:

```bash
# 1. Generate new keypair
./scripts/setup-validator.sh --layer=l2 --rotate

# 2. Submit key rotation governance proposal (or contact chain governance)
pantheon-cli governance propose \
    --layer=l2 \
    --type=PARAM_CHANGE \
    --title="Rotate validator key for <your_address>" \
    --description="New pubkey: <new_pubkey>"

# 3. After proposal passes, import new key
pantheon-cli validator keys-import --layer=l2 --pubkey=<new_pubkey>

# 4. Securely destroy old key material
```

### 3.3 Backup

```bash
# Daily backup of node state (run as cron job)
./scripts/backup.sh /var/lib/pantheon/l2 \
    /mnt/backup/pantheon-l2-$(date +%Y%m%d).tar.gz
```

Backups include: chainstate, wallet, mempool snapshot, config.

### 3.4 Restore from Backup

```bash
sudo systemctl stop pantheon-l2
./scripts/restore.sh /mnt/backup/pantheon-l2-YYYYMMDD.tar.gz \
    /var/lib/pantheon/l2
sudo systemctl start pantheon-l2
# Verify node re-syncs from the restored height
pantheon-cli node sync-status --layer=l2
```

---

## 4. Incident Response

### 4.1 Node Stopped / Not Producing Blocks

1. Check logs: `journalctl -fu pantheon-l2 -n 200`
2. Check disk: `df -h /var/lib/pantheon/l2`
3. Check peers: `pantheon-cli node peer-info --layer=l2`
4. Restart: `sudo systemctl restart pantheon-l2`
5. If persistent: restore from last known-good backup.

### 4.2 Validator Jailed (Slashed)

A validator is jailed when it misses too many block proposals or double-signs.

```bash
# Verify slashing event
pantheon-cli governance list --layer=l2  # look for SLASH proposals

# After fixing the root cause (usually clock skew or key misconfiguration):
# 1. Fix the issue
# 2. Wait for unjail period (see genesis_drachma.json → slashing_params)
# 3. Re-stake if needed
pantheon-cli staking deposit --layer=l2 --amount=<amount> --address=<addr>
```

**Common causes of slashing**:
- System clock drift (use `chronyc tracking` to verify NTP sync)
- Double-signing (running two validators with the same key — never do this)
- Extended downtime exceeding the liveness threshold

### 4.3 Double-Sign Prevention

- **Never** run the same validator key on two hosts simultaneously.
- Use a signing sidecar or HSM that enforces single-signer policy.
- Before starting a restored node, ensure the original is fully stopped and
  the data directory is from a confirmed single checkpoint.

### 4.4 Peer Count Drops to Zero

```bash
# Check firewall allows P2P port (default 9333 for L2)
sudo ufw status
sudo ufw allow 9333/tcp

# Check bootstrap nodes reachable
for node in $(python3 -c "import json; [print(n) for n in json.load(open('configs/mainnet/l2.json'))['p2p_bootstrap_nodes']]"); do
    nc -zv $(echo $node | tr ':' ' ') 2>&1
done

# Force peer re-discovery by restarting
sudo systemctl restart pantheon-l2
```

---

## 5. Claiming Rewards

```bash
# Check pending rewards
pantheon-cli staking rewards --layer=l2 --address=<your_address>

# Claim (triggers an on-chain transaction)
pantheon-cli staking withdraw --layer=l2 \
    --amount=<reward_amount> \
    --address=<your_address>
```

Rewards accrue every epoch (see `genesis_drachma.json` → `epoch_length`).

---

## 6. Exiting the Validator Set

```bash
# 1. Signal intent to unstake (enters unbonding period)
pantheon-cli staking withdraw \
    --layer=l2 \
    --amount=<full_staked_amount> \
    --address=<your_address>

# 2. Wait for unbonding period to expire (see genesis_drachma.json → unbonding_epochs)
# 3. Withdraw unlocked tokens
pantheon-cli staking rewards --layer=l2 --address=<your_address>

# 4. Stop the node once removed from active set
sudo systemctl stop pantheon-l2
sudo systemctl disable pantheon-l2
```

---

## 7. Useful Reference

| Command | Description |
|---------|-------------|
| `journalctl -fu pantheon-l2` | Live node logs |
| `pantheon-cli node sync-status --layer=l2` | Sync and height |
| `pantheon-cli node peer-info --layer=l2` | Connected peers |
| `pantheon-cli staking status --layer=l2 --address=X` | Stake and jail status |
| `pantheon-cli staking rewards --layer=l2 --address=X` | Pending rewards |
| `curl http://127.0.0.1:9332/health` | Raw health check |
| `curl http://127.0.0.1:9332/metrics` | Prometheus metrics |

---

*Last updated: 2026-03-09*
*Applies to: PantheonChain v2.3.0+*
