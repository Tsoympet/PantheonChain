#!/usr/bin/env bash
set -euo pipefail

echo "Running shell syntax checks"
bash -n scripts/*.sh tests/integration/*.sh

echo "Lint completed"
