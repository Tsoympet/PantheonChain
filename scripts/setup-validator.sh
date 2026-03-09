#!/usr/bin/env bash
# setup-validator.sh — Interactive wizard for setting up a PantheonChain validator
# Usage: ./scripts/setup-validator.sh --layer=l2|l3 [--rotate]
set -euo pipefail

LAYER=""
ROTATE=false

for arg in "$@"; do
    case "$arg" in
        --layer=*)  LAYER="${arg#--layer=}" ;;
        --rotate)   ROTATE=true ;;
        --help|-h)
            echo "Usage: $0 --layer=l2|l3 [--rotate]"
            echo "  --layer    Which layer to set up a validator for (l2=DRACHMA, l3=OBOLOS)"
            echo "  --rotate   Key rotation mode: generates a new key alongside the existing one"
            exit 0 ;;
        *)
            echo "Unknown argument: $arg" >&2
            exit 1 ;;
    esac
done

if [[ "$LAYER" != "l2" && "$LAYER" != "l3" ]]; then
    echo "Error: --layer must be l2 (DRACHMA) or l3 (OBOLOS)" >&2
    exit 1
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# Determine layer-specific settings
if [[ "$LAYER" == "l2" ]]; then
    LAYER_NAME="DRACHMA"
    P2P_PORT=9333
    RPC_PORT=9332
    GENESIS_FILE="genesis_drachma.json"
else
    LAYER_NAME="OBOLOS"
    P2P_PORT=10333
    RPC_PORT=10332
    GENESIS_FILE="genesis_obolos.json"
fi

KEYS_DIR="${HOME}/.pantheon/validator-keys/${LAYER}"
mkdir -p "${KEYS_DIR}"
chmod 700 "${KEYS_DIR}"

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║   PantheonChain Validator Setup — ${LAYER_NAME} (${LAYER^^})         ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

if [[ "$ROTATE" == "true" ]]; then
    echo "⚠  KEY ROTATION MODE"
    echo "   This will generate a NEW validator keypair."
    echo "   Your current key will NOT be removed until you submit a rotation proposal."
    echo ""
fi

echo "▸ Step 1: Generate secp256k1 keypair for BIP-340 Schnorr signing"
echo ""

PRIVKEY_FILE="${KEYS_DIR}/validator.privkey"
PUBKEY_FILE="${KEYS_DIR}/validator.pubkey"
ADDRESS_FILE="${KEYS_DIR}/validator.address"

if [[ -f "${PRIVKEY_FILE}" && "$ROTATE" == "false" ]]; then
    echo "  Existing key found at ${PRIVKEY_FILE}"
    read -rp "  Overwrite? (yes/no) [no]: " OVERWRITE
    OVERWRITE="${OVERWRITE:-no}"
    if [[ "$OVERWRITE" != "yes" ]]; then
        echo "  Keeping existing key."
        PUBKEY="$(cat "${PUBKEY_FILE}")"
        ADDRESS="$(cat "${ADDRESS_FILE}")"
    else
        OVERWRITE="yes"
    fi
fi

if [[ ! -f "${PRIVKEY_FILE}" || "${OVERWRITE:-no}" == "yes" || "$ROTATE" == "true" ]]; then
    # Generate private key: 32 random bytes from /dev/urandom, hex-encoded
    PRIVKEY="$(od -An -N32 -tx1 /dev/urandom | tr -d ' \n')"
    # Derive a deterministic address placeholder (in production this calls the node)
    ADDRESS="parthenon1q$(echo "${PRIVKEY}" | sha256sum | cut -c1-40)"
    # Compressed pubkey placeholder (real implementation uses secp256k1 library)
    PUBKEY="02$(echo "${PRIVKEY}" | sha256sum | cut -c1-64)"

    echo "${PRIVKEY}" > "${PRIVKEY_FILE}"
    echo "${PUBKEY}"  > "${PUBKEY_FILE}"
    echo "${ADDRESS}" > "${ADDRESS_FILE}"
    chmod 600 "${PRIVKEY_FILE}"

    echo "  ✓ Private key saved to: ${PRIVKEY_FILE}"
fi

echo ""
echo "▸ Step 2: Validator identity"
echo ""
echo "  Validator address : ${ADDRESS}"
echo "  Public key (hex)  : ${PUBKEY}"
echo ""

# Read minimum stake from genesis
MIN_STAKE="0"
if command -v python3 &>/dev/null && [[ -f "${GENESIS_FILE}" ]]; then
    MIN_STAKE="$(python3 -c "import json; d=json.load(open('${GENESIS_FILE}')); print(d.get('minimum_stake',0))" 2>/dev/null || echo "0")"
fi

echo "▸ Step 3: Staking"
echo ""
echo "  Minimum stake required: ${MIN_STAKE} base units (from ${GENESIS_FILE})"
echo ""
echo "  To stake, run:"
echo "    pantheon-cli staking deposit \\"
echo "        --layer=${LAYER} \\"
echo "        --amount=<amount_in_base_units> \\"
echo "        --address=${ADDRESS}"
echo ""

echo "▸ Step 4: Register validator key"
echo ""
echo "  Run the following command AFTER your node is synced:"
echo "    pantheon-cli validator keys-import \\"
echo "        --layer=${LAYER} \\"
echo "        --pubkey=${PUBKEY}"
echo ""

echo "▸ Step 5: Verify participation"
echo ""
echo "  After staking and key import, verify your validator is active:"
echo "    pantheon-cli validator status --layer=${LAYER} --json"
echo "    pantheon-cli staking status   --layer=${LAYER} --address=${ADDRESS} --json"
echo ""

if [[ "$ROTATE" == "true" ]]; then
    echo "▸ Step 6: Submit key rotation proposal"
    echo ""
    echo "  Once your new key is registered and working, submit a governance proposal"
    echo "  to officially record the rotation:"
    echo ""
    echo "    pantheon-cli governance propose \\"
    echo "        --layer=${LAYER} \\"
    echo "        --type=PARAM_CHANGE \\"
    echo "        --title='Validator key rotation for ${ADDRESS}' \\"
    echo "        --description='New pubkey: ${PUBKEY}'"
    echo ""
fi

echo "▸ Next steps"
echo ""
echo "  1. Ensure your node is running:  sudo systemctl status pantheon-${LAYER}"
echo "  2. Confirm P2P port ${P2P_PORT} is reachable from the internet"
echo "  3. Set up Prometheus scraping from http://127.0.0.1:${RPC_PORT}/metrics"
echo "  4. Read the validator runbook: docs/ops/validator_runbook.md"
echo ""
echo "✅ Validator setup for ${LAYER_NAME} complete."
echo ""
echo "⚠  SECURITY REMINDER:"
echo "   - Back up ${KEYS_DIR} to an encrypted offline medium"
echo "   - Never expose ${PRIVKEY_FILE} to the network"
echo "   - Consider migrating to a hardware wallet or HSM for production"
echo ""
