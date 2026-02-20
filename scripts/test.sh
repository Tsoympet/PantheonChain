#!/usr/bin/env bash
set -euo pipefail
ctest --test-dir build --output-on-failure
./scripts/run-devnet.sh
trap 'xargs -r kill < <(cat .devnet/*.pid) || true' EXIT
./tests/integration/devnet_smoke_test
