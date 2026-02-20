# Legacy relayers build path

This directory is a compatibility shim so existing build scripts that call `add_subdirectory(relayers)` continue to work.

Active relayer source files are now located under `src/relayers/`:

- `src/relayers/pantheon-relayer-l2.cpp`
- `src/relayers/pantheon-relayer-l3.cpp`

New feature development should target `src/relayers`.
