# CLI

`pantheon-cli` supports layer-aware operational commands.

## Layered commands

```bash
pantheon-cli stake deposit --layer=l2
pantheon-cli deploy-contract --layer=l3
pantheon-cli submit-commitment --layer=l2 --commitment=<encoded_l3_or_l2_commitment>
pantheon-cli submit-commitment --layer=l3 --commitment=<encoded_l3_commitment>
```

## Notes

- `stake deposit` is valid on DRACHMA (L2) only.
- `deploy-contract` is valid on OBOLOS (L3) only.
- Commitments must use the canonical format:
  `SOURCE:epoch:finalized_height:finalized_block_hash:state_root:validator_set_hash:validator|stake|sig,...`
