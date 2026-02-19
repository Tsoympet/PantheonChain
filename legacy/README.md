# Legacy Layout Notes

This directory documents components from the pre-layered repository layout that are still present for compatibility.

- `layer1/` and `layer2/` remain in-tree to avoid breaking existing build targets and tests.
- New layered modules live under `src/common`, `src/talanton`, `src/drachma`, `src/obolos`, `src/relayers`, and `src/tools`.
- New documentation and runtime paths should use the layered structure and `configs/` profiles.

Migration work should prefer new layered paths first.
