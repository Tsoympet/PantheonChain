#!/usr/bin/env bash
set -euo pipefail
bash -n scripts/*.sh tests/integration/*
python3 -m py_compile scripts/devnet_layer_server.py scripts/validate-config.py scripts/lint-docs-terminology.py
./scripts/placeholder-gate.sh
python3 scripts/lint-docs-terminology.py
echo "Lint completed"
