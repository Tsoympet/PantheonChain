# PantheonChain – Operator Runbook Index

This directory contains production-grade operational runbooks for PantheonChain node operators,
security teams, and on-call engineers.

## Runbooks

| Runbook | Description |
|---------|-------------|
| [backup-restore.md](backup-restore.md) | Full node backup and point-in-time restore procedures |
| [key-compromise.md](key-compromise.md) | Key compromise response and validator key rotation |
| [incident-response.md](incident-response.md) | Security incident classification, containment, and post-mortem |

## Severity Levels

| Level | Description | Response time |
|-------|-------------|---------------|
| P0 – Critical | Network halt, consensus failure, fund loss | Immediate / 15 min |
| P1 – High | Chain fork, RPC outage, governance attack | 1 hour |
| P2 – Medium | Single validator down, degraded sync | 4 hours |
| P3 – Low | Performance degradation, non-critical alerts | Next business day |

## On-Call Contacts

Configure your on-call rotation in your internal PagerDuty / OpsGenie instance and keep this
document up to date with the current escalation path.

## Prerequisites for All Runbooks

- SSH access to the node host with `sudo` privilege.
- Access to the node's data directory (default: `/var/lib/pantheonchain`).
- `pantheon-cli` binary available in `$PATH`.
- Read access to the governance RPC endpoint.
