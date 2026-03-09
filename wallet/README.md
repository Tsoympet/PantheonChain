# Wallet — Pantheon Unified Wallet

This directory contains the unified Pantheon wallet implementation that
supports all three PantheonChain layers.

## Features

The Pantheon wallet provides:

- **Multi-chain accounts** — a single seed phrase manages addresses on
  TALANTON (L1), DRACHMA (L2), and OBOLOS (L3)
- **TLT balance** — TALANTON Layer 1 native token
- **DRC balance** — DRACHMA Layer 2 native token
- **OBL balance** — OBOLOS Layer 3 native token
- **Wrapped asset balances** — wTLT on DRACHMA, wDRC on OBOLOS
- **Cross-chain transfer UI** — initiate bridge transfers between chains

## Address Derivation

PantheonChain uses BIP-44 derivation paths with chain-specific coin types:

| Chain | Address Prefix | BIP-44 Path |
|-------|---------------|------------|
| TALANTON (L1) | `tlt` | `m/44'/1001'/0'/0/n` |
| DRACHMA (L2)  | `drc` | `m/44'/1002'/0'/0/n` |
| OBOLOS (L3)   | `obl` | `m/44'/1003'/0'/0/n` |

## Chain ID Collision Prevention

Each chain has a unique `chain_id` in its genesis file:

| Chain | Chain ID | Network ID |
|-------|---------|-----------|
| TALANTON | 1001 | 1001 |
| DRACHMA  | 1002 | 1002 |
| OBOLOS   | 1003 | 1003 |

Transactions signed for one chain cannot be replayed on another chain
because the `chain_id` is included in the transaction hash preimage.

## Desktop Wallet

See [`../clients/desktop/`](../clients/desktop/) for the Qt-based desktop wallet.

## Mobile Wallet

See [`../clients/mobile/react-native/`](../clients/mobile/react-native/) for
the React Native mobile wallet.

## CLI Wallet

See [`../clients/cli/`](../clients/cli/) for the command-line wallet interface.

## Signing

The wallet uses **BIP-340 Schnorr signatures** (secp256k1) for all chains.
See the `WalletService` in the mobile client for the reference implementation.
