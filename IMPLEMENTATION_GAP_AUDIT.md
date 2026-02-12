# ParthenonChain Implementation Gap Audit

This document summarizes what is still **TODO**, **placeholder**, **stubbed**, or **not wired into the main build**, based on a repository scan.

## 1) Critical gaps vs. project claims

The project currently claims a fully production-ready implementation with no placeholders, but multiple components still contain placeholders or stubs.

- README claims: "Complete implementation with no placeholders".
- Whitepaper claims: "Production-ready implementation - No placeholders".

## 2) Cryptography and storage dependencies still fall back to stubs

### Current state

- Root CMake conditionally builds stubbed `secp256k1`, `leveldb`, `cpp-httplib`, and `json` if real submodules are missing.
- `third_party/README.md` explicitly marks secp256k1 and LevelDB as placeholder-only status and says production requires proper initialization.

### What must be done for full implementation

1. Enforce real dependency presence in production builds (fail configure if stubs are selected).
2. Remove/guard stub fallback paths for release pipeline.
3. Add CI gate ensuring `third_party/secp256k1` and `third_party/leveldb` are real implementations, not stubs.

## 3) P2P protocol messages are partially unimplemented

### Current state

`layer1/core/p2p/message.cpp` still throws `std::logic_error("... not implemented")` for:

- `AddrMessage::{Serialize,Deserialize}`
- `BlockMessage::{Serialize,Deserialize}`
- `TxMessage::{Serialize,Deserialize}`
- `GetHeadersMessage::{Serialize,Deserialize}`
- `HeadersMessage::{Serialize,Deserialize}`

### What must be done for full implementation

1. Implement full wire-format serialization/deserialization for all message types.
2. Add negative/edge-case tests (malformed lengths, overflows, truncated payloads).
3. Add compatibility tests against canonical network vectors.

## 4) Hardware cryptography contains placeholder behavior

### Current state

In `layer1/core/crypto/hardware_crypto.cpp`:

- AES encrypt/decrypt paths currently use `memcpy` placeholder semantics instead of actual AES transforms.
- Comments explicitly indicate simplified behavior and future real AES-NI intrinsics.
- "GPU" signature verification uses deterministic CPU fallback semantics.

### What must be done for full implementation

1. Implement real AES mode (e.g., AES-256-GCM/CTR with authentication).
2. Replace placeholder copy behavior with cryptographically correct transform.
3. Implement real GPU backend or rename module to deterministic batch verifier to avoid misleading behavior.
4. Add correctness tests with known-answer vectors and cross-implementation checks.

## 5) Wallet hardware security flows are mock/simplified

### Current state

In `layer1/wallet/hardware/firmware_verification.cpp`:

- Vendor keys are dummy byte arrays.
- Signature verification is simplified (`size` checks) rather than cryptographic verification.
- Update checking/downloading/loading DB are placeholders.
- Install flow returns failure and many checks return fixed values.

### What must be done for full implementation

1. Load trusted vendor keys from signed, versioned trust store.
2. Implement real signature verification for firmware metadata and images.
3. Implement secure update metadata retrieval + authenticity checks.
4. Implement device-specific install protocol with rollback protection.
5. Add integration tests for tampered firmware, revoked keys, downgrade attacks.

## 6) Mobile SDK is mostly scaffold/mock behavior

### Current state

In `tools/mobile_sdks/mobile_sdk.cpp`:

- Wallet generation/derivation/signing use fixed dummy data.
- RPC/network calls return hard-coded synthetic responses.
- Subscription APIs are no-op.
- Secure storage methods are placeholder behavior.

### What must be done for full implementation

1. Replace fixed keys/mnemonics with secure RNG and standards-compliant derivation.
2. Implement authenticated RPC client calls and error handling.
3. Implement WebSocket/event subscriptions and reconnect logic.
4. Integrate platform secure storage (Keychain/Keystore equivalents).
5. Add end-to-end tests against a running node.

## 7) Layer-2 implementation is partial and not fully wired

### Current state

In `layer2/rollups/zk_rollup.cpp` there are explicit production placeholders for:

- Merkle proof generation/verification
- zk-SNARK proof verification/generation
- batch compression/decompression
- trusted setup and exit proof checks

Additionally, `layer2/CMakeLists.txt` only builds a subset of layer-2 modules and does **not** include several existing source files (e.g., optimistic rollup, plasma chain), meaning parts of the repository are present but not part of the built `layer2` target.

### What must be done for full implementation

1. Implement complete state-tree and proof systems.
2. Replace proof placeholders with audited proving/verification backends.
3. Add missing layer2 modules to the build graph (or remove dead code).
4. Add integration tests for batch lifecycle, exits, and fraud/validity proofs.

## 8) Developer tools appear present but not integrated in primary build

### Current state

Multiple tool directories exist (`tools/*`) but root CMake does not add a `tools` subtree. This suggests many utilities are either manually built or currently unused by default build/test pipelines.

### What must be done for full implementation

1. Decide supported tools and expose them through CMake targets.
2. Add CI jobs for supported tooling binaries.
3. Remove or archive experimental tools that are not intended for release.

## 9) Suggested completion roadmap (priority order)

1. **P0:** Remove dependency stubs from production path (crypto/storage correctness).
2. **P0:** Complete P2P message implementations and protocol tests.
3. **P1:** Replace crypto placeholders (AES/GPU labeling + firmware verification).
4. **P1:** Wire/build all intended layer2 modules and complete proof plumbing.
5. **P2:** Promote mobile/tooling from scaffolds to production integrations.
6. **P2:** Add CI quality gates that fail on `"not implemented"`, known placeholder comments, or stub fallback selection.

---

## Definition of "full implementation" for this repository

A practical completion target should include:

- No runtime `"not implemented"` paths in consensus/network critical flows.
- No cryptographic or persistence stubs in production builds.
- Every committed production module either built+tested, or explicitly marked experimental and excluded from release.
- CI checks that prevent regression into placeholder behavior.
