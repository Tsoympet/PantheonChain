# Migration

## What changes for existing users

- Node operators now choose an explicit layer role: `--layer=l1|l2|l3`.
- Configs are profile-based under `configs/`.
- Devnet orchestration uses three node instances plus relayers.
- RPC and CLI now include layered commitment/staking/execution command families.

## Backward compatibility

Legacy folders (`layer1/`, `layer2/`) remain for compatibility; new work should target layered `src/` modules.
