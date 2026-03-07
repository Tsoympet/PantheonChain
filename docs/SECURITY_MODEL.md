# Security Model (Reference)

This document summarizes MVP security posture and aligns with `docs/threat-model.md` and `docs/SETTLEMENT_AND_FINALITY.md`.

## Security Layers

### TALANTON (L1)

- Security source: SHA-256d PoW.
- Finality type: probabilistic.
- Role: root settlement anchor and strongest trust-minimized layer.

### DRACHMA (L2)

- Security source: PoS/BFT validator quorum signatures.
- Finality type: economic finality under validator honesty assumptions.
- Key risks: validator collusion, key compromise, delayed checkpoint publication.

### OBOLOS (L3)

- Security source: PoS/BFT validator quorum signatures.
- Finality type: economic finality under validator honesty assumptions.
- Key risks: validator collusion, execution bugs, delayed checkpoint publication.

## Commitment Security

Canonical checkpoint path:

`OBOLOS -> DRACHMA -> TALANTON`

`TX_L3_COMMIT` and `TX_L2_COMMIT` checks are quorum-signature and payload-validation based in MVP.

## MVP Limitations

- No zk proofs.
- No fraud proofs.
- No claim of trustless bridging.
- Bridge withdrawals are optimistic and include a trust window.
- Relayers provide liveness, not root consensus safety.

## Operator Guidance

- Monitor checkpoint freshness at each layer boundary.
- Enforce hardened key management for PoS validator infrastructure.
- Use conservative TALANTON confirmation depth for high-value settlement actions.
