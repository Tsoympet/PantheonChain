#!/usr/bin/env bash
set -euo pipefail

ctest --test-dir build -R "LayersTests|devnet_smoke_rpc" --output-on-failure
./scripts/run-devnet.sh
trap 'xargs -r kill < <(cat .devnet/*.pid) || true' EXIT
sleep 1
./tests/integration/devnet-smoke.sh
