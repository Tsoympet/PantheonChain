# ParthenonChain Cryptographic Primitives

This directory contains consensus-critical cryptographic implementations for ParthenonChain.

## ⚠️ CONSENSUS CRITICAL

All code in this module is part of the consensus rules. Any changes must be carefully reviewed and coordinated across the entire network.

## Implemented Cryptographic Functions

### SHA-256
- Standard SHA-256 hash function (FIPS 180-4 compliant)
- Deterministic and consensus-safe
- Used for various hashing operations throughout the blockchain

**Files:**
- `sha256.h` / `sha256.cpp`

**Test Vectors:**
- NIST test vectors
- Bitcoin Core compatibility tests

### SHA-256d (Double SHA-256)
- SHA256d(x) = SHA256(SHA256(x))
- Used for block hashing and proof-of-work
- Bitcoin-compatible implementation

### Tagged SHA-256
- BIP-340 style tagged hashing
- Format: TaggedHash(tag, msg) = SHA256(SHA256(tag) || SHA256(tag) || msg)
- Prevents cross-protocol attacks by domain separation

### Schnorr Signatures
- BIP-340 compliant Schnorr signatures
- Uses secp256k1 elliptic curve
- X-only public keys (32 bytes)
- 64-byte signatures (r || s)
- Deterministic signing with optional auxiliary randomness

**Files:**
- `schnorr.h` / `schnorr.cpp`

**Features:**
- Key generation from private keys
- Message signing with BIP-340
- Signature verification
- Deterministic nonce generation
- Optional auxiliary randomness for additional security

## Dependencies

- **libsecp256k1**: Bitcoin Core's secp256k1 library
  - Location: `third_party/secp256k1/`
  - Modules enabled: `extrakeys`, `schnorrsig`

## Building

The crypto library is built as part of the main ParthenonChain build:

```bash
mkdir build && cd build
cmake ..
make parthenon_crypto
```

## Testing

All cryptographic functions have comprehensive unit tests with deterministic test vectors:

```bash
# Run all crypto tests
ctest -R crypto

# Run specific tests
./tests/unit/crypto/test_sha256
./tests/unit/crypto/test_schnorr
```

### Test Coverage

**SHA-256:**
- Empty string
- Standard test vectors
- Long messages
- Incremental hashing
- Double SHA-256
- Tagged SHA-256
- Bitcoin genesis block verification
- Large data (1MB+)

**Schnorr:**
- Private key validation
- Public key derivation
- Sign and verify
- Deterministic signing
- Invalid signature rejection
- Auxiliary randomness
- Batch signatures

## Security Considerations

1. **Determinism**: All operations are fully deterministic (no randomness from system time or random devices in consensus paths)
2. **Constant-time**: The secp256k1 library provides constant-time operations to prevent timing attacks
3. **Memory safety**: Careful memory management to prevent leaks of private keys
4. **Test vectors**: Extensive test coverage ensures correctness

## Future Work (Upcoming Phases)

This module will be used by:
- Transaction signing and verification (Phase 2)
- Block hashing (Phase 3)
- Merkle tree construction (Phase 2)
- P2P message authentication (Phase 5)

## References

- [FIPS 180-4](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf) - SHA-256 specification
- [BIP-340](https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki) - Schnorr signatures for secp256k1
- [libsecp256k1](https://github.com/bitcoin-core/secp256k1) - Bitcoin Core secp256k1 library
