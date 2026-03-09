# Security Policy

## Reporting Vulnerabilities

Please report vulnerabilities **privately** before public disclosure.

- **Preferred**: [GitHub Security Advisories](https://github.com/Tsoympet/PantheonChain/security/advisories/new) for this repository.
- **Alternative**: Email `security@pantheonchain.org` with PGP encryption (key on Keybase).
- Include: impact, affected components, reproduction steps, environment, and suggested remediation.

Do **not** open a public issue or pull request for a security vulnerability.

## Scope

The following components are in scope:

| Component | Description |
|-----------|-------------|
| Node daemon (`parthenond`) | All three layer nodes (L1/L2/L3) |
| RPC server | JSON-RPC API, HTTP endpoints, auth |
| CLI (`pantheon-cli`) | All sub-commands |
| Relayers | L2→L1 and L3→L2 commitment relayers |
| Consensus | PoW, PoS/BFT, slashing, finality |
| Cryptography | SHA-256d, Schnorr/secp256k1, key management |
| Smart contracts (EVM) | L3 Obolos execution engine |
| Cross-layer bridge | Commitment anchoring and fraud proofs |
| Mobile wallet | React Native client, signing |
| CI/CD pipeline | Workflow injection, supply-chain |

**Out of scope**: Third-party infrastructure, social engineering, DoS without a meaningful exploit.

## Severity Classification

| Severity | Description | Examples |
|----------|-------------|---------|
| **Critical** | Remote code execution, consensus split, unlimited token minting, private key extraction | RCE via RPC, double-spend that bypasses all checks |
| **High** | Large-scale fund theft, permanent network disruption, validator slashing bypass | Fraudulent commitment accepted by L1, >1% supply inflation |
| **Medium** | Limited fund theft, transaction censorship, temporary DoS, information leak | Mempool manipulation, partial replay attack |
| **Low** | Minor information disclosure, rate-limit bypass, UI spoofing | Version fingerprinting, non-critical log injection |
| **Informational** | Best-practice gaps with no direct exploit path | Missing HTTP headers, verbose error messages |

## Response SLA

| Severity | Acknowledgement | Patch Target | Public Disclosure |
|----------|----------------|-------------|-------------------|
| Critical | 24 hours | 7 days | Coordinated, after patch |
| High | 48 hours | 14 days | Coordinated, after patch |
| Medium | 5 business days | 30 days | 90 days from report |
| Low | 10 business days | 90 days | 120 days from report |
| Informational | 15 business days | Best effort | 180 days from report |

The PantheonChain team reserves the right to accelerate or delay disclosure in
coordination with the reporter and dependent ecosystem projects.

## Security Hardening Checklist

Before each mainnet release:

- [ ] Independent security audit by recognized firm
- [ ] Fuzzing campaign (AFL++ / libFuzzer) on parser surfaces
- [ ] Static analysis (CodeQL, clang-tidy) passes with zero Critical/High findings
- [ ] All cryptographic keys use hardware-backed storage (HSM or Ledger) for genesis validators
- [ ] RPC endpoints require authentication (`rpc.allow_unauthenticated=false`)
- [ ] All P2P and RPC communications use TLS 1.3 in production
- [ ] Validator key rotation procedure documented and rehearsed
- [ ] Slashing evidence is reproducible and has test coverage

## Known Limitations

- The L3→L2→L1 bridge currently uses optimistic fraud proofs with a challenge window.
  It does not yet provide zk-proof-based instant finality. See `docs/SETTLEMENT_AND_FINALITY.md`.
- Hardware wallet firmware verification requires operator-supplied vendor keys (see
  `layer1-talanton/wallet/hardware/firmware_verification.cpp`). Placeholder keys are
  **not** included in the binary; operators must supply their own via `vendor_keys.json`.

## Hall of Fame

Responsible disclosures will be acknowledged here (with reporter's consent) after patches ship.

