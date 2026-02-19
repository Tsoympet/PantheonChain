# Relayer Components

PantheonChain ships two relayer binaries that implement the canonical commitment path:

- `pantheon-relayer-l3`: relays `TX_L3_COMMIT` objects from OBOLOS to DRACHMA.
- `pantheon-relayer-l2`: relays `TX_L2_COMMIT` objects from DRACHMA to TALANTON.

Both binaries validate commitment payload encoding and finality quorum preconditions before forwarding.

## CLI usage

```bash
pantheon-relayer-l3 \
  --commitment=OBOLOS:<epoch>:<height>:<block_hash>:<state_root>:<validator_set_hash>:<validator_id>|<stake>|<signature>(,...)> \
  --active-stake=<active_stake> \
  --last-finalized-height=<last_seen_l3_height>

pantheon-relayer-l2 \
  --commitment=DRACHMA:<epoch>:<height>:<block_hash>:<state_root>:<validator_set_hash>:<validator_id>|<stake>|<signature>(,...)> \
  --active-stake=<active_stake> \
  --last-finalized-height=<last_seen_l2_height>
```

A successful invocation emits a normalized encoded commitment on stdout for downstream transport.
