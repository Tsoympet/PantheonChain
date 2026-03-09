# TALANTON Chain (Layer 1 — Root Chain)

**Chain ID:** 1001  
**Network ID:** 1001  
**Address Prefix:** `tlt`  
**Consensus:** Proof-of-Work (SHA256d)  
**Native Token:** TLT (TALANTON)  
**RPC Port:** 8332  
**P2P Port:** 8333  

## Role

TALANTON is the **root settlement chain** of the PantheonChain network. It provides:

- **Security anchoring** for the entire three-layer stack
- **Final settlement** for cross-chain transactions
- **Checkpoint storage** for DRACHMA (L2) state roots

TALANTON does **not** depend on DRACHMA or OBOLOS for its security. All other chains
derive their ultimate security from TALANTON's PoW consensus.

## Token Economics

| Parameter | Value |
|-----------|-------|
| Symbol | TLT |
| Initial Block Reward | 50 TLT |
| Halving Interval | 210,000 blocks |
| Max Supply | 21,000,000 TLT |
| Epoch Length | 144 blocks |

## Cross-Chain Relationships

- Receives checkpoint anchors from **DRACHMA (L2)**
- Locks TLT → mints **wTLT** on DRACHMA via the L1↔L2 bridge

## Source References

- Genesis: [`genesis_talanton.json`](../../genesis_talanton.json)
- Consensus implementation: [`layer1-talanton/consensus/`](../../layer1-talanton/core/consensus/)
- Node: [`layer1-talanton/node/`](../../layer1-talanton/node/)
- Bridge: [`bridge/l1_l2/`](../../bridge/l1_l2/)
