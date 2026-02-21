# Testnet Operations Runbook

## Build Binaries

```bash
./scripts/build.sh
```

## Generate/Validate Configs

```bash
python3 scripts/validate-config.py configs/testnet/l1.json configs/testnet/l2.json configs/testnet/l3.json
```

## Start Nodes + Relayers (Devnet profile)

```bash
./scripts/run-devnet.sh
```

## Logs

- Node logs: `.devnet/logs/l1.log`, `.devnet/logs/l2.log`, `.devnet/logs/l3.log`
- Integration output: terminal + CI artifacts on failure

## Backup

```bash
./scripts/backup.sh .devnet backups/devnet-$(date +%Y%m%d%H%M%S).tar.gz
```

## Restore

```bash
./scripts/restore.sh backups/devnet-YYYYMMDDHHMMSS.tar.gz .devnet
```

## Upgrade Procedure (Testnet Rolling Restart)

1. Build new binaries on all nodes.
2. Stop one node at a time.
3. Replace binary and restart node.
4. Verify `/health` and `/chain/info` before moving to next node.

## Incident Quick Guide

- Node stuck: check process + logs, restart affected node, verify health.
- Relayer stalled: inspect relayer logs and restart relayer process.
- RPC down: verify port binding and restart service.
