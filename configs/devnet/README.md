# Devnet Configuration

This directory contains the configuration files for the PantheonChain local development network.

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
| L1 TALANTON | 18333 | 18332 |
| L2 DRACHMA  | 19333 | 19332 |
| L3 OBOLOS   | 20333 | 20332 |

## Differences from Testnet

- `network` is set to `devnet` (testnet uses `testnet`)
- `rpc.allow_unauthenticated` is `true` (testnet uses `false`)
- Data directories are under `.devnet/` (testnet uses `.testnet/`)
- Ports are in the 18xxx–20xxx range (testnet uses 28xxx–30xxx)
- Mining is disabled by default for faster iteration

## Quickstart

```bash
./scripts/run-devnet.sh
./tests/integration/devnet-smoke.sh
```
