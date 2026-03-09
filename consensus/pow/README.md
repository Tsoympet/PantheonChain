# Proof-of-Work Consensus (TALANTON — Layer 1)

TALANTON uses **SHA256d Proof-of-Work**, identical in structure to Bitcoin's
PoW algorithm but with PantheonChain-specific parameters.

## Parameters

| Parameter | Value |
|-----------|-------|
| Algorithm | SHA256d (double SHA-256) |
| Block time target | ~10 minutes |
| Difficulty adjustment | Every 144 blocks (epoch) |
| Initial block reward | 50 TLT |
| Halving interval | 210,000 blocks |
| Max supply | 21,000,000 TLT |

## Difficulty Adjustment

The difficulty is re-targeted every epoch (144 blocks) to maintain the
10-minute block time target. The adjustment is clamped to ±4× the previous
difficulty to prevent oscillation.

## Mining Algorithm

Block header includes:
- `version` (4 bytes)
- `prev_block_hash` (32 bytes, SHA256d)
- `merkle_root` (32 bytes)
- `timestamp` (4 bytes, Unix)
- `bits` (4 bytes, compact difficulty target)
- `nonce` (4 bytes)

A block is valid when `SHA256d(header) < target`.

## Block Reward Schedule

| Height range | Block reward |
|-------------|-------------|
| 0 – 209,999 | 50 TLT |
| 210,000 – 419,999 | 25 TLT |
| 420,000 – 629,999 | 12.5 TLT |
| … | … |

## Source Implementation

- Header: [`../../layer1-talanton/core/consensus/difficulty.h`](../../layer1-talanton/core/consensus/difficulty.h)
- Difficulty: [`../../layer1-talanton/core/consensus/difficulty.cpp`](../../layer1-talanton/core/consensus/difficulty.cpp)
- Issuance: [`../../layer1-talanton/core/consensus/issuance.h`](../../layer1-talanton/core/consensus/issuance.h)
- Genesis: [`../../layer1-talanton/core/consensus/genesis.h`](../../layer1-talanton/core/consensus/genesis.h)
