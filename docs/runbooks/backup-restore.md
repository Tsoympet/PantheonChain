# PantheonChain – Backup and Restore Runbook

**Severity:** Maintenance procedure (no active incident required).

---

## 1. Overview

This runbook covers:
- Full node state backup (cold snapshot).
- Incremental / warm backup with continuous replication.
- Point-in-time restore from backup.
- Post-restore verification.

**Data locations (defaults):**

| Component | Path |
|-----------|------|
| Block storage (LevelDB) | `/var/lib/pantheonchain/blocks/` |
| UTXO / chainstate | `/var/lib/pantheonchain/chainstate/` |
| Governance state | `/var/lib/pantheonchain/governance/` |
| Wallet keys (encrypted) | `/var/lib/pantheonchain/wallet/` |
| Node configuration | `/etc/pantheonchain/pantheonchain.conf` |

---

## 2. Pre-Backup Checklist

- [ ] Confirm sufficient disk space on backup destination (`df -h <dest>`).
- [ ] Note current block height: `pantheon-cli getblockcount`.
- [ ] Verify the node is healthy: `pantheon-cli getinfo | jq .syncing`.

---

## 3. Cold Backup (Recommended for Archival)

A cold backup stops the daemon, snapshots the data directory, and restarts.

```bash
# 1. Stop the node gracefully (waits for in-flight writes to flush).
sudo systemctl stop pantheonchain

# 2. Create a timestamped archive.
BACKUP_DIR="/backups/pantheonchain"
TIMESTAMP=$(date +%Y%m%dT%H%M%SZ)
sudo mkdir -p "${BACKUP_DIR}"
sudo tar -czf "${BACKUP_DIR}/pantheonchain-${TIMESTAMP}.tar.gz" \
    /var/lib/pantheonchain \
    /etc/pantheonchain/pantheonchain.conf

# 3. Record the block height at backup time.
echo "${TIMESTAMP}: $(cat /var/lib/pantheonchain/blocks/.height 2>/dev/null || echo unknown)" \
    >> "${BACKUP_DIR}/backup-index.log"

# 4. Restart the node.
sudo systemctl start pantheonchain

# 5. Verify the node came back and is syncing.
sleep 5 && pantheon-cli getinfo
```

---

## 4. Warm Backup (Online Snapshot with LevelDB Checkpoint)

The LevelDB backend supports live checkpoints.  Use `pantheon-cli` to trigger a
checkpoint before copying the files.

```bash
# Request a checkpoint from the daemon.
pantheon-cli checkpoint create --dir /tmp/pantheonchain-checkpoint

# Copy the checkpoint to the backup destination.
TIMESTAMP=$(date +%Y%m%dT%H%M%SZ)
rsync -a --delete /tmp/pantheonchain-checkpoint/ \
    /backups/pantheonchain/checkpoint-${TIMESTAMP}/

# Optionally compress.
tar -czf /backups/pantheonchain/pantheonchain-checkpoint-${TIMESTAMP}.tar.gz \
    /backups/pantheonchain/checkpoint-${TIMESTAMP}/
rm -rf /backups/pantheonchain/checkpoint-${TIMESTAMP}/
```

> **Note:** The `checkpoint create` RPC is implemented in the storage layer.
> Verify it is available in your node version with `pantheon-cli help checkpoint`.

---

## 5. Incremental Backup with rsync

For frequent incremental snapshots without downtime:

```bash
# Run from a cron job (e.g. every 15 minutes):
rsync -a --checksum \
    /var/lib/pantheonchain/blocks/ \
    /backups/pantheonchain/incremental/blocks/

rsync -a --checksum \
    /var/lib/pantheonchain/chainstate/ \
    /backups/pantheonchain/incremental/chainstate/
```

Keep at least 7 days of hourly incremental snapshots and 30 days of daily cold backups.

---

## 6. Restore Procedure

### 6.1 Restore from Cold Backup

```bash
# 1. Stop the node.
sudo systemctl stop pantheonchain

# 2. Remove existing data (save config first).
sudo cp /etc/pantheonchain/pantheonchain.conf /tmp/pantheonchain.conf.bak
sudo rm -rf /var/lib/pantheonchain

# 3. Extract the backup.
sudo tar -xzf /backups/pantheonchain/pantheonchain-<TIMESTAMP>.tar.gz -C /

# 4. Restore config if overwritten.
sudo cp /tmp/pantheonchain.conf.bak /etc/pantheonchain/pantheonchain.conf

# 5. Verify file ownership.
sudo chown -R pantheonchain:pantheonchain /var/lib/pantheonchain

# 6. Restart and verify.
sudo systemctl start pantheonchain
sleep 10 && pantheon-cli getinfo
```

### 6.2 Partial Restore (Chainstate Only)

If block storage is intact but chainstate is corrupted:

```bash
sudo systemctl stop pantheonchain
sudo rm -rf /var/lib/pantheonchain/chainstate
sudo tar -xzf /backups/pantheonchain/pantheonchain-<TIMESTAMP>.tar.gz \
    -C /var/lib/pantheonchain chainstate
sudo chown -R pantheonchain:pantheonchain /var/lib/pantheonchain/chainstate
sudo systemctl start pantheonchain
# The node will re-index from blocks if chainstate is missing.
```

---

## 7. Post-Restore Verification

Run the following checks after any restore:

```bash
# 1. Confirm the node started.
pantheon-cli getinfo

# 2. Confirm block height is plausible (not stuck at genesis).
HEIGHT=$(pantheon-cli getblockcount)
echo "Block height after restore: ${HEIGHT}"

# 3. Check peer connectivity.
pantheon-cli getinfo | jq .connections

# 4. Run the smoke test suite.
cd /opt/pantheonchain && ./tests/devnet_smoke_rpc --endpoint http://127.0.0.1:8332
```

---

## 8. Backup Rotation Policy

| Backup type | Retention | Storage location |
|-------------|-----------|-----------------|
| Cold (daily) | 30 days | Off-site object storage |
| Warm checkpoint (hourly) | 7 days | Local NVMe |
| Incremental rsync (15 min) | 48 hours | Local NVMe |

Use `find /backups/pantheonchain -name "*.tar.gz" -mtime +30 -delete` in a cron job to
enforce the 30-day cold-backup retention policy.
