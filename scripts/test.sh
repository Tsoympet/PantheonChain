#!/usr/bin/env bash
set -euo pipefail
./scripts/run-devnet.sh
trap 'xargs -r kill < <(cat .devnet/*.pid) || true' EXIT
ctest --test-dir build --output-on-failure
./tests/integration/devnet_smoke_test
