# Node Operations

## Data pruning

Use dedicated data directories per layer (`.devnet/l1`, `.devnet/l2`, `.devnet/l3`) and archive before pruning.

## Backups

- Backup wallet seed files and configs.
- Validate restore procedure on non-production environments.

## Key management

- Validator and relayer keys must be isolated and access-controlled.
- Never commit credentials in configs.
- Use authenticated RPC in non-dev environments.
