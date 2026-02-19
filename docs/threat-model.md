# Threat Model (MVP)

## Security assumptions

This MVP uses rollup-style commitments with economic finality signatures.

- No zk-proofs.
- No fraud proofs.
- Trust model depends on >=2/3 honest active stake in L2 and L3 validator sets.

## Known limitations

- Bridge withdrawals are optimistic with a trust window.
- Commitment validity is signature/quorum based, not proof-system based.
- Relayer censorship/delay is possible but detectable from missing commitments.
