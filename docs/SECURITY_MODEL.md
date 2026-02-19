# Security Model (Reference)

This file aligns with the canonical MVP threat assumptions in `docs/threat-model.md`.

## Security Layers

### TALANTON (L1)

- Security source: SHA-256d PoW economic security.
- Role: immutable settlement anchor and root of trust.
- No staking, no BFT validator quorum in consensus.

### DRACHMA (L2)

- Security source: PoS+BFT finality with >=2/3 active stake signatures.
- Risks: validator collusion, key compromise, delayed commitments.

### OBOLOS (L3)

- Security source: PoS+BFT finality with >=2/3 active stake signatures.
- Risks: validator collusion, execution bugs, delayed commitment publication.

## Commitment Security

Anchoring path is strictly:

`OBOLOS -> DRACHMA -> TALANTON`

`TX_L3_COMMIT` and `TX_L2_COMMIT` validation is signature-quorum and encoding based in MVP.

## MVP Limitations

- No zk proofs.
- No fraud proofs.
- Bridge withdrawals are optimistic and include a trust window.
- Relayers are liveness components; they do not replace consensus safety.

## Operator Guidance

- Monitor commitment freshness at each layer boundary.
- Enforce secure validator key management (HSM/signing isolation recommended).
- Treat bridge unlocks as delayed-finality operations.

For the normative and complete threat model, refer to `docs/threat-model.md`.
