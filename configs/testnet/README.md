# Testnet Configuration

This directory contains the configuration files for the PantheonChain public testnet.

## Files

| File | Description |
|------|-------------|
| `l1.json` | L1 (TALANTON) node JSON config |
| `l1.conf` | L1 (TALANTON) daemon config |
| `l2.json` | L2 (DRACHMA) node JSON config |
| `l2.conf` | L2 (DRACHMA) daemon config |
| `l3.json` | L3 (OBOLOS) node JSON config |
| `l3.conf` | L3 (OBOLOS) daemon config |

## Port Assignments

| Layer | P2P Port | RPC Port |
|-------|----------|----------|
| L1 TALANTON | 28333 | 28332 |
| L2 DRACHMA  | 29333 | 29332 |
| L3 OBOLOS   | 30333 | 30332 |

## Differences from Devnet

- `network` is set to `testnet` (devnet uses `devnet`)
- `rpc.allow_unauthenticated` is `false` (devnet uses `true`)
- Data directories are under `.testnet/` (devnet uses `.devnet/`)
- Ports are in the 28xxx–30xxx range (devnet uses 18xxx–20xxx)

## Quickstart

```bash
./scripts/run-testnet.sh
./tests/integration/testnet-smoke.sh
```
