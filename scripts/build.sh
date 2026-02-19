#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPARTHENON_REQUIRE_REAL_DEPS=OFF
cmake --build build -j"$(nproc)"
