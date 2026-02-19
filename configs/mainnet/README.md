# Mainnet Config Templates

This directory provides **production templates** for TALANTON (L1), DRACHMA (L2), and OBOLOS (L3):

- `l1.json`, `l2.json`, `l3.json` for JSON-based orchestration and validation.
- `l1.conf`, `l2.conf`, `l3.conf` for daemon runtime flags.

Before launch:

1. Replace `node_id` values with operator identities.
2. Set hardened `data_dir` paths with backup and pruning policy.
3. Confirm P2P/RPC ports do not overlap with host services.
4. Lock genesis references to audited, signed genesis artifacts.
5. Run `python3 scripts/validate-config.py configs/mainnet/l1.json configs/mainnet/l2.json configs/mainnet/l3.json`.
