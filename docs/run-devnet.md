# Run Devnet

## Local process mode

1. Build binaries:
   ```bash
   ./scripts/build.sh
   ```
2. Validate configs:
   ```bash
   ./scripts/repo-audit.sh
   ```
3. Start layered devnet (mock RPC harness + relayer binaries):
   ```bash
   ./scripts/run-devnet.sh
   ```
4. Run smoke/integration checks:
   ```bash
   ./scripts/test.sh
   ```

## Docker mode

```bash
docker compose up --build
```

Services: `talanton-l1`, `drachma-l2`, `obolos-l3`, `relayer-l2`, `relayer-l3`.


> Note: the current devnet script runs a deterministic mock RPC harness for L1/L2/L3 to provide stable CI smoke coverage for layered RPC/commitment flows.
