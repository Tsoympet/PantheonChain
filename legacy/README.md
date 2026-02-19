# Legacy Layout Notes

This directory tracks compatibility paths from the pre-layered repository structure.

Legacy/compatibility roots that may still exist in-tree:

- `layer1/` and `layer2/`
- top-level `tools/` utilities not yet migrated to `src/tools`
- historical docs with uppercase naming retained for backwards links

Current feature development should target the layered layout:

- `src/common`
- `src/talanton`
- `src/drachma`
- `src/obolos`
- `src/relayers`
- `src/tools`

If you must touch a compatibility path, document why in your PR and add a migration note in `docs/migration.md` when user-facing.
