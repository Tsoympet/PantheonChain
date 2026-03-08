#!/usr/bin/env bash
set -euo pipefail
./scripts/run-devnet.sh
trap 'xargs -r kill < <(cat .devnet/*.pid) || true' EXIT
ctest --test-dir build --output-on-failure
./tests/integration/devnet_smoke_test
python3 scripts/runtime/checkpoint_watchdog.py --oneshot
./tests/integration/adversarial-multinode.sh
