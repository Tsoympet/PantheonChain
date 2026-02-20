#!/usr/bin/env bash
set -euo pipefail
bash -n scripts/*.sh tests/integration/*
echo "Lint completed"
