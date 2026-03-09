# DRACHMA Chain (Layer 2 — Economic Chain)

**Chain ID:** 1002  
**Network ID:** 1002  
**Address Prefix:** `drc`  
**Consensus:** Proof-of-Stake / BFT  
**Native Token:** DRC (DRACHMA)  
**RPC Port:** 9332  
**P2P Port:** 9333  

## Role

DRACHMA is the **economic chain** of the PantheonChain network. It handles:

- **Payments and liquidity** for everyday transactions
- **Validator staking** for PoS consensus
- **Checkpoint aggregation** from OBOLOS (L3) → committed to TALANTON (L1)

## Token Economics

| Parameter | Value |
|-----------|-------|
| Symbol | DRC |
| Initial Block Reward | 97 DRC (9,700,000,000 base units) |
| Halving Interval | 210,000 blocks |
| Max Supply | ~2,037,000,000 DRC |
| Minimum Stake | 100,000 DRC |
| Double-Sign Slash | 5% |
| Equivocation Slash | 10% |

## Cross-Chain Relationships

- Commits checkpoint anchors to **TALANTON (L1)**
- Receives checkpoint anchors from **OBOLOS (L3)**
- Holds **wTLT** (wrapped TLT locked on TALANTON)
- Locks DRC → mints **wDRC** on OBOLOS via the L2↔L3 bridge

## Security

DRACHMA's security is derived from:
1. Its own PoS validator set
2. Ultimately, from checkpoint anchors committed to TALANTON (PoW root)

## Source References

- Genesis: [`genesis_drachma.json`](../../genesis_drachma.json)
- Consensus implementation: [`layer2-drachma/consensus/`](../../layer2-drachma/consensus/)
- Node: [`layer2-drachma/node/`](../../layer2-drachma/node/)
- Bridge L1↔L2: [`bridge/l1_l2/`](../../bridge/l1_l2/)
- Bridge L2↔L3: [`bridge/l2_l3/`](../../bridge/l2_l3/)
