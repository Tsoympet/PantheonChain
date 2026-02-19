# CLI

`parthenon-cli` supports legacy and layered commands.

## Layered commands

```bash
parthenon-cli stake deposit --layer=l2
parthenon-cli deploy-contract --layer=l3
parthenon-cli submit-commitment --layer=l2
```

## Legacy commands

```bash
parthenon-cli getinfo
parthenon-cli getblockcount
parthenon-cli getbalance TALANTON
```
