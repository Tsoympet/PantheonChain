# Tokenomics

PantheonChain uses a three-token model with non-overlapping primary consensus roles:

- **TALANTON (L1):** PoW mining reward asset, base-layer security token, L1 settlement/fee asset.
- **DRACHMA (L2):** PoS staking collateral and fee utility for the payments/checkpoint layer.
- **OBOLOS (L3):** PoS staking, gas, governance, and execution-economy asset.

Security hierarchy is settlement-oriented:

`OBOLOS < DRACHMA < TALANTON`

Upper-layer economic finality does not replace TALANTON base settlement.

Parameters are network-profile specific in genesis/config files:

- `genesis_talanton.json`
- `genesis_drachma.json`
- `genesis_obolos.json`
