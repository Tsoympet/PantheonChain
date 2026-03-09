#!/usr/bin/env bash
# run-mainnet.sh — Start a PantheonChain mainnet node stack
# Usage: ./scripts/run-mainnet.sh [--layer=l1|l2|l3|all] [--no-relayers]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

LAYER="all"
NO_RELAYERS=false

for arg in "$@"; do
    case "$arg" in
        --layer=*) LAYER="${arg#--layer=}" ;;
        --no-relayers) NO_RELAYERS=true ;;
        --help|-h)
            echo "Usage: $0 [--layer=l1|l2|l3|all] [--no-relayers]"
            exit 0 ;;
        *)
            echo "Unknown argument: $arg" >&2; exit 1 ;;
    esac
done

# ------------------------------------------------------------------ #
#  Pre-flight checks                                                   #
# ------------------------------------------------------------------ #
echo "▸ Pre-flight checks"

# Validate all mainnet configs
python3 scripts/validate-config.py \
    configs/mainnet/l1.json configs/mainnet/l2.json configs/mainnet/l3.json
echo "  ✓ Config validation passed"

python3 scripts/validate-layer-model.py
echo "  ✓ Layer model validation passed"

python3 scripts/economics/check_supply_caps.py | grep -q "PASS All supply cap checks"
echo "  ✓ Supply cap checks passed"

# Check binaries exist
for bin in build/pantheon-l1-talanton-node build/pantheon-l2-drachma-node \
           build/pantheon-l3-obolos-node build/pantheon-cli; do
    if [[ ! -x "$bin" ]]; then
        echo "  ✗ Missing binary: $bin — run ./scripts/build.sh first" >&2
        exit 1
    fi
done
echo "  ✓ All binaries present"

# Check genesis files exist
for f in genesis_talanton.json genesis_drachma.json genesis_obolos.json; do
    if [[ ! -f "$f" ]]; then
        echo "  ✗ Missing genesis file: $f" >&2; exit 1
    fi
done
echo "  ✓ Genesis files present"

# ------------------------------------------------------------------ #
#  Data directories                                                    #
# ------------------------------------------------------------------ #
DATA_ROOT="${PANTHEON_DATA_DIR:-/var/lib/pantheon}"
LOG_DIR="${PANTHEON_LOG_DIR:-/var/log/pantheon}"

mkdir -p "${DATA_ROOT}/l1" "${DATA_ROOT}/l2" "${DATA_ROOT}/l3"
mkdir -p "${LOG_DIR}"

# ------------------------------------------------------------------ #
#  Cleanup handler                                                     #
# ------------------------------------------------------------------ #
PIDS=()
cleanup() {
    echo ""
    echo "▸ Shutting down PantheonChain mainnet stack..."
    for pid in "${PIDS[@]}"; do
        kill "$pid" 2>/dev/null || true
    done
    wait 2>/dev/null || true
    echo "  Done."
}
trap cleanup EXIT INT TERM

# ------------------------------------------------------------------ #
#  Helper: wait for RPC health                                         #
# ------------------------------------------------------------------ #
wait_for_rpc() {
    local url="$1" name="$2"
    echo -n "  Waiting for ${name} RPC at ${url} ..."
    for _ in {1..60}; do
        if curl -sf "${url}/health" >/dev/null 2>&1; then
            echo " ready"
            return 0
        fi
        sleep 1
        echo -n "."
    done
    echo " TIMEOUT" >&2
    return 1
}

# ------------------------------------------------------------------ #
#  Start L1                                                            #
# ------------------------------------------------------------------ #
if [[ "$LAYER" == "all" || "$LAYER" == "l1" ]]; then
    echo ""
    echo "▸ Starting L1 TALANTON node..."
    build/pantheon-l1-talanton-node \
        --layer=l1 \
        --config=configs/mainnet/l1.conf \
        >"${LOG_DIR}/l1.log" 2>&1 &
    PIDS+=($!)
    echo "  PID: ${PIDS[-1]}  Log: ${LOG_DIR}/l1.log"
    wait_for_rpc "http://127.0.0.1:8332" "L1"
fi

# ------------------------------------------------------------------ #
#  Start L2                                                            #
# ------------------------------------------------------------------ #
if [[ "$LAYER" == "all" || "$LAYER" == "l2" ]]; then
    echo ""
    echo "▸ Starting L2 DRACHMA node..."
    build/pantheon-l2-drachma-node \
        --layer=l2 \
        --config=configs/mainnet/l2.conf \
        >"${LOG_DIR}/l2.log" 2>&1 &
    PIDS+=($!)
    echo "  PID: ${PIDS[-1]}  Log: ${LOG_DIR}/l2.log"
    wait_for_rpc "http://127.0.0.1:9332" "L2"
fi

# ------------------------------------------------------------------ #
#  Start L3                                                            #
# ------------------------------------------------------------------ #
if [[ "$LAYER" == "all" || "$LAYER" == "l3" ]]; then
    echo ""
    echo "▸ Starting L3 OBOLOS node..."
    build/pantheon-l3-obolos-node \
        --layer=l3 \
        --config=configs/mainnet/l3.conf \
        >"${LOG_DIR}/l3.log" 2>&1 &
    PIDS+=($!)
    echo "  PID: ${PIDS[-1]}  Log: ${LOG_DIR}/l3.log"
    wait_for_rpc "http://127.0.0.1:10332" "L3"
fi

# ------------------------------------------------------------------ #
#  Start relayers                                                      #
# ------------------------------------------------------------------ #
if [[ "$NO_RELAYERS" == "false" && "$LAYER" == "all" ]]; then
    echo ""
    echo "▸ Starting commitment relayers..."

    if [[ -x build/relayers/pantheon-relayer-l2 ]]; then
        build/relayers/pantheon-relayer-l2 \
            >"${LOG_DIR}/relayer-l2.log" 2>&1 &
        PIDS+=($!)
        echo "  relayer-l2 PID: ${PIDS[-1]}  Log: ${LOG_DIR}/relayer-l2.log"
    else
        echo "  ⚠  build/relayers/pantheon-relayer-l2 not found — skipping"
    fi

    if [[ -x build/relayers/pantheon-relayer-l3 ]]; then
        build/relayers/pantheon-relayer-l3 \
            >"${LOG_DIR}/relayer-l3.log" 2>&1 &
        PIDS+=($!)
        echo "  relayer-l3 PID: ${PIDS[-1]}  Log: ${LOG_DIR}/relayer-l3.log"
    else
        echo "  ⚠  build/relayers/pantheon-relayer-l3 not found — skipping"
    fi
fi

# ------------------------------------------------------------------ #
#  Start checkpoint watchdog                                           #
# ------------------------------------------------------------------ #
if [[ "$LAYER" == "all" ]]; then
    FRESHNESS="$(python3 -c "import json; print(json.load(open('configs/mainnet/l1.json'))['checkpoint_freshness_slo_seconds'])")"
    python3 scripts/runtime/checkpoint_watchdog.py \
        --l1-state "${DATA_ROOT}/l1" \
        --l2-state "${DATA_ROOT}/l2" \
        --freshness-seconds "${FRESHNESS}" \
        --interval-seconds 30 \
        >"${LOG_DIR}/watchdog.log" 2>&1 &
    PIDS+=($!)
    echo ""
    echo "  Checkpoint watchdog started (SLO: ${FRESHNESS}s)  Log: ${LOG_DIR}/watchdog.log"
fi

# ------------------------------------------------------------------ #
#  Status summary                                                      #
# ------------------------------------------------------------------ #
echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║   PantheonChain Mainnet Stack Running                ║"
echo "╠══════════════════════════════════════════════════════╣"
[[ "$LAYER" == "all" || "$LAYER" == "l1" ]] && \
echo "║   L1 TALANTON   RPC: http://127.0.0.1:8332          ║"
[[ "$LAYER" == "all" || "$LAYER" == "l2" ]] && \
echo "║   L2 DRACHMA    RPC: http://127.0.0.1:9332          ║"
[[ "$LAYER" == "all" || "$LAYER" == "l3" ]] && \
echo "║   L3 OBOLOS     RPC: http://127.0.0.1:10332         ║"
echo "║                                                      ║"
echo "║   Logs: ${LOG_DIR}                     ║"
echo "║   Stop: Ctrl-C                                       ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""

# Wait for all children
wait
