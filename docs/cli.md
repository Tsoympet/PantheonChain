# CLI

`pantheon-cli` supports layer-aware operational commands.

## Layered commands

```bash
pantheon-cli stake deposit --layer=l2
pantheon-cli deploy-contract --layer=l3
pantheon-cli submit-commitment --layer=l2 --commitment=<encoded_l2_commitment>
pantheon-cli submit-commitment --layer=l3 --commitment=<encoded_l3_commitment>
```

## Monetary denomination commands

```bash
pantheon-cli getbalance DRACHMA
pantheon-cli chain/monetary_spec
pantheon-cli sendtoaddress TALANTON <addr> 1.25 --in-tal
pantheon-cli sendtoaddress DRACHMA <addr> 10 --in-dr
pantheon-cli sendtoaddress OBOLOS <addr> 42.50000000 --in-ob
```

`sendtoaddress` parses display amounts with explicit denomination flags:

- `--in-tal` (TALANTON)
- `--in-dr` (DRACHMA)
- `--in-ob` (OBOLOS)

Optional Attic-style denomination override:

- `--denom=tetradrachm`
- `--denom=mina`
- positional denom after amount, e.g. `sendtoaddress DRACHMA <addr> 2 tetradrachm --in-dr`

CLI output uses dual representation where possible: integer raw amount + formatted unit amount.

## Notes

- `stake deposit` is valid on DRACHMA (L2) only.
- `deploy-contract` is valid on OBOLOS (L3) only.
- Commitments must use the canonical format:
  `SOURCE:epoch:finalized_height:finalized_block_hash:state_root:validator_set_hash:upstream_commitment_hash:validator|stake|sig,...`
- Monetary ratios are protocol-level accounting only:
  - `1 DRACHMA = 6 OBOLOS`
  - `1 TALANTON = 6000 DRACHMA = 36000 OBOLOS`

## Examples

```bash
pantheon-cli sendtoaddress DRACHMA <addr> 2 tetradrachm --in-dr
pantheon-cli sendtoaddress DRACHMA <addr> 5 --denom=mina --in-dr
# gas estimate in obol; report in drachma
pantheon-cli getbalance OBOLOS
```
