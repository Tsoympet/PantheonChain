# Contracts — PantheonChain Smart Contracts

This directory contains smart contracts deployed on the OBOLOS (L3) EVM chain,
as well as bridge contracts for the L1 and L2 chains.

## Structure

```
contracts/
├── README.md
├── bridge/                  — Bridge contracts (lock/unlock/mint/burn)
│   ├── TalantonBridge.sol   — L1 bridge contract (lock TLT, unlock TLT)
│   ├── DrachmaBridge.sol    — L2 bridge contract (mint wTLT, burn wTLT,
│   │                           lock DRC, unlock DRC)
│   └── ObolosBridge.sol     — L3 bridge contract (mint wDRC, burn wDRC)
├── governance/              — On-chain governance (OBOLOS only)
│   ├── Governor.sol         — Proposal creation, voting, execution
│   └── TimelockController.sol
└── tokens/                  — Token standards
    ├── WrappedTLT.sol       — wTLT ERC-20 on DRACHMA
    └── WrappedDRC.sol       — wDRC ERC-20 on OBOLOS
```

## Deployment Addresses

Contract addresses are assigned at genesis or via governance proposals.
See each chain's genesis file for pre-deployed contracts.

## Token Sovereignty

Per PantheonChain architecture rules:

- **TLT** is native on TALANTON only. Bridge contracts on DRACHMA hold
  wTLT (an ERC-20 representation), not native TLT.
- **DRC** is native on DRACHMA only. Bridge contracts on OBOLOS hold
  wDRC (an ERC-20 representation), not native DRC.
- **OBL** is native on OBOLOS only. There are no wrapped OBL tokens.

## Development

Smart contracts targeting OBOLOS must use Solidity ≥ 0.8.0 and are
compiled with the standard EVM toolchain. The OBOLOS EVM is compatible
with Ethereum Homestead/Berlin opcodes.

See [`../layer3-obolos/evm/`](../layer3-obolos/evm/) for the EVM implementation.
