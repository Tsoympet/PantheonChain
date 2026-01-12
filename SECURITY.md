# Security Policy

## Overview

ParthenonChain is a production-grade blockchain system with consensus-critical components. Security is our highest priority.

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |
| < 1.0   | :x:                |

## Security Model

ParthenonChain implements a defense-in-depth security model:

### Layer 1 (Consensus Critical)

**Determinism**: All consensus code is fully deterministic with no reliance on:
- System time (except for block timestamps which are validated)
- Random number generation (except for optional auxiliary randomness in signatures)
- External data sources
- Non-deterministic system calls

**Cryptographic Security**:
- SHA-256d Proof-of-Work (Bitcoin-compatible)
- Schnorr signatures (BIP-340) on secp256k1
- Constant-time cryptographic operations
- Memory-safe key handling

**Validation**:
- All transactions are fully validated before acceptance
- Block validation includes PoW verification, transaction validation, and state transition checks
- UTXO set integrity is maintained through Merkle tree commitments
- Smart contract execution is deterministic and gas-limited

### Layer 2 (Non-Consensus)

Layer 2 components are isolated from consensus:
- Payment channel disputes are settled on Layer 1
- Indexers and APIs cannot affect blockchain state
- Bridges use cryptographic proofs (SPV, HTLC)

### Network Security

- P2P message authentication
- DoS protection through rate limiting
- Peer reputation scoring
- Eclipse attack mitigation

## Reporting a Vulnerability

**Please DO NOT file a public issue for security vulnerabilities.**

### Responsible Disclosure Process

1. **Contact**: Send details to security@parthenonchain.org (if available) or create a private security advisory on GitHub
2. **Information to Include**:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if any)
   - Your contact information

3. **Response Timeline**:
   - **24-48 hours**: Initial acknowledgment
   - **7 days**: Preliminary assessment and severity classification
   - **30 days**: Fix development and testing
   - **60 days**: Coordinated disclosure (or earlier if fix is deployed)

4. **Severity Classification**:
   - **Critical**: Consensus failure, loss of funds, network split
   - **High**: DoS, privacy breach, non-consensus fund loss
   - **Medium**: Limited impact vulnerabilities
   - **Low**: Theoretical or difficult-to-exploit issues

### Bug Bounty

We appreciate responsible disclosure and may offer rewards for:
- Consensus-breaking vulnerabilities
- Cryptographic implementation flaws
- Smart contract execution issues
- Network protocol vulnerabilities

Rewards are assessed on a case-by-case basis depending on:
- Severity and impact
- Quality of report
- Ease of exploitation
- Whether the vulnerability was previously known

## Security Best Practices for Users

### Node Operators

1. **Keep software updated**: Always run the latest stable release
2. **Secure configuration**:
   - Use strong RPC passwords
   - Restrict RPC access to localhost or trusted IPs only
   - Enable firewall rules
3. **Backup private keys**: Store wallet backups securely offline
4. **Monitor logs**: Watch for unusual activity
5. **Use checksums**: Verify installer integrity before running

### Wallet Users

1. **Verify addresses**: Double-check recipient addresses before sending
2. **Secure private keys**: Never share private keys or seed phrases
3. **Use hardware wallets**: For large amounts, consider hardware wallet integration
4. **Beware of phishing**: Always download from official sources
5. **Test with small amounts**: Test transactions with small amounts first

### Developers

1. **Review consensus code**: All consensus changes require extensive review
2. **Add tests**: Comprehensive test coverage for new features
3. **Follow coding standards**: Use deterministic, memory-safe patterns
4. **Isolate non-consensus code**: Keep Layer 2 code separate from Layer 1
5. **Run static analysis**: Use tools to detect potential vulnerabilities

## Known Limitations

### Current Version (1.0.0)

- **No hardware wallet support**: Integration planned for future releases
- **Limited multi-sig UI**: Command-line only for complex multi-sig workflows
- **No built-in Tor support**: Manual configuration required for Tor usage

## Security Audits

- **Phase 1 (Cryptographic Primitives)**: Internal review completed
- **Phase 2-10**: Comprehensive testing during development
- **External Audit**: Recommended before mainnet launch

## Cryptographic Dependencies

- **libsecp256k1**: Bitcoin Core's battle-tested elliptic curve library
- **OpenSSL**: For TLS and additional cryptographic functions (non-consensus)

## Incident Response

In case of a security incident:

1. **Detection**: Monitoring systems detect anomalies
2. **Assessment**: Security team evaluates severity
3. **Containment**: Immediate actions to prevent further damage
4. **Patch Development**: Fix is developed and tested
5. **Deployment**: Emergency release if necessary
6. **Disclosure**: Public disclosure after fix is widely deployed
7. **Post-Mortem**: Analysis and process improvements

## Security Contact

- **Email**: security@parthenonchain.org (create this if public launch planned)
- **GitHub Security Advisories**: [https://github.com/Tsoympet/PantheonChain/security/advisories](https://github.com/Tsoympet/PantheonChain/security/advisories)
- **PGP Key**: [TBD - publish release signing key fingerprint]

## Additional Resources

- [Security Model Documentation](docs/SECURITY_MODEL.md)
- [Architecture Overview](docs/ARCHITECTURE.md)
- [Consensus Rules](docs/LAYER1_CORE.md)

---

**Last Updated**: 2026-01-12

**Security is everyone's responsibility. Thank you for helping keep ParthenonChain secure.**
