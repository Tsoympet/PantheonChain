# PantheonChain – Key Compromise Response Runbook

**Severity:** P0 – Immediate response required.

---

## 1. Overview

This runbook covers:
- Detecting a potential key compromise.
- Isolating the affected validator / wallet.
- Rotating the compromised key on-chain.
- Post-rotation verification.

Key types in PantheonChain:

| Key type | Used for | Rotation method |
|----------|----------|-----------------|
| Validator signing key (Schnorr) | Block signing | Emergency governance proposal |
| Node network identity key | P2P identity | Node config update + restart |
| Governance voting key | Proposal submission / voting | Key rotation proposal |
| Wallet spending key (hardware) | Fund custody | Hardware wallet replacement |
| Emergency council key | M-of-N multi-sig | Council quorum re-election |

---

## 2. Detection Signals

Investigate immediately if you observe:
- Unexplained blocks signed by your validator key from an IP you don't control.
- Governance votes or proposals you did not authorise.
- Unexpected treasury withdrawals citing your proposal ID.
- Your hardware wallet vendor reports a firmware vulnerability.
- A third party provides a signed message proving access to your private key material.

---

## 3. Immediate Containment (First 15 minutes)

```bash
# 1. If the node is actively signing blocks, STOP it immediately.
sudo systemctl stop pantheonchain

# 2. Disconnect the machine from the network to prevent further use of the key.
sudo ip link set <interface> down

# 3. Take a forensic snapshot of memory and disk before any key material changes.
sudo dd if=/dev/mem of=/forensics/mem.dd bs=4096 2>/dev/null || true
sudo tar -czf /forensics/node-snapshot-$(date +%Y%m%dT%H%M%SZ).tar.gz \
    /var/lib/pantheonchain /etc/pantheonchain /var/log/pantheonchain

# 4. Notify on-call security lead and governance council.
# (Use your organisation's incident notification channel.)
```

---

## 4. Validator Key Rotation

### 4.1 Generate a new signing key

On a fresh, air-gapped machine:

```bash
pantheon-cli keygen --type schnorr --output /secure/new-validator-key.json
# Record the new public key (32-byte x-only hex):
NEW_PUBKEY=$(cat /secure/new-validator-key.json | jq -r .public_key)
echo "New validator public key: ${NEW_PUBKEY}"
```

### 4.2 Submit an emergency validator key update proposal

The Emergency Council (M-of-N guardians) can fast-track this:

```bash
# Build execution_data for PARAMETER_CHANGE: key="validator_pubkey" value=<new pubkey>
# Encode: [key_len(1 byte)][key_bytes][new_pubkey_le8(8 bytes)] – see governance spec.
pantheon-cli governance submit_proposal \
  --type EMERGENCY \
  --title "Emergency validator key rotation for node <NODE_ID>" \
  --description "Rotating compromised validator signing key" \
  --proposer <YOUR_PROPOSER_ADDRESS_HEX> \
  --execution_data <ENCODED_KEY_ROTATION_DATA>
```

### 4.3 Emergency Council fast-track approval

Contact the Prytany (rotating executive sub-committee) to fast-track the proposal:

```bash
# List current Prytany members:
pantheon-cli governance boule prytany

# Each guardian signs the emergency action:
pantheon-cli emergency sign \
  --action FAST_TRACK_UPGRADE \
  --proposal_id <PROPOSAL_ID> \
  --signer <GUARDIAN_ADDRESS>
```

### 4.4 Apply the new key to the node config

After the proposal is executed:

```bash
# Update the node config.
sudo nano /etc/pantheonchain/pantheonchain.conf
# Set: validator_signing_key = /secure/new-validator-key.json

# Restart the node.
sudo systemctl start pantheonchain
```

---

## 5. Wallet Key Compromise (Hardware Wallet)

If a hardware wallet is lost, stolen, or compromised:

1. **Immediately revoke spending authority** – if using a multi-sig policy, use the remaining
   signers to move funds to a new address controlled by a new hardware device.
2. **Report to the vendor** – file a theft/loss report with Ledger or Trezor.
3. **Update the governance key** – if the hardware wallet was used for governance voting,
   submit a key-update proposal as above.
4. **Audit recent transactions** – check the block explorer for any unauthorised spends from
   the compromised address.

---

## 6. Emergency Council Key Compromise

If a council guardian key is compromised, reducing the M-of-N quorum below the minimum:

1. The remaining council members vote to remove the compromised guardian via
   `pantheon-cli governance boule remove_citizen`.
2. A new election is triggered via `pantheon-cli governance boule sortition`.
3. The new council member's key is registered on-chain.

---

## 7. Post-Rotation Checklist

- [ ] New key is active on-chain (verify with `pantheon-cli getinfo | jq .validator_pubkey`).
- [ ] Old key is revoked / no longer produces valid block signatures.
- [ ] Governance and emergency council updated their key registries.
- [ ] Forensic analysis of the compromise vector completed.
- [ ] Incident post-mortem filed (see `incident-response.md`).
- [ ] Hardware security review performed if a hardware wallet was involved.
- [ ] Vendor notified if a firmware vulnerability was involved.
