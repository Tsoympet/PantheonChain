# OBOLOS Chain (Layer 3 — Execution Chain)

**Chain ID:** 1003  
**Network ID:** 1003  
**Address Prefix:** `obl`  
**Consensus:** Proof-of-Stake / BFT  
**Native Token:** OBL (OBOLOS)  
**RPC Port:** 10332  
**P2P Port:** 10333  

## Role

OBOLOS is the **execution chain** of the PantheonChain network. It provides:

- **EVM smart contract execution** (Ethereum-compatible VM)
- **On-chain governance** via voting and proposals
- **Gas accounting** using OBL as the gas token
- **Checkpoint production** → committed to DRACHMA (L2)

## Token Economics

| Parameter | Value |
|-----------|-------|
| Symbol | OBL |
| Initial Block Reward | 145 OBL (14,500,000,000 base units) |
| Halving Interval | 210,000 blocks |
| Max Supply | ~3,045,000,000 OBL |
| Minimum Stake | 50,000 OBL |
| Double-Sign Slash | 5% |
| Equivocation Slash | 10% |
| Gas Token | OBL |

## Cross-Chain Relationships

- Commits checkpoint anchors to **DRACHMA (L2)**
- Holds **wDRC** (wrapped DRC locked on DRACHMA)
- Smart contracts can bridge tokens to/from DRACHMA

## Security

OBOLOS security derives from:
1. Its own PoS validator set
2. Checkpoint anchors on DRACHMA (L2)
3. Ultimately, from TALANTON (L1) PoW root via the checkpoint chain

## Source References

- Genesis: [`genesis_obolos.json`](../../genesis_obolos.json)
- Consensus implementation: [`layer3-obolos/consensus/`](../../layer3-obolos/consensus/)
- EVM implementation: [`layer3-obolos/evm/`](../../layer3-obolos/evm/)
- Node: [`layer3-obolos/node/`](../../layer3-obolos/node/)
- Bridge L2↔L3: [`bridge/l2_l3/`](../../bridge/l2_l3/)
