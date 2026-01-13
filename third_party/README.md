# Third-Party Dependencies - Build Instructions

## Status

The PantheonChain project uses several third-party libraries as git submodules. These are located in the `third_party/` directory.

## Required Dependencies

### 1. secp256k1 (CRITICAL)
**Purpose:** Elliptic curve cryptography for Schnorr signatures  
**Status:** ⚠️ Submodule placeholder only  
**Required for:** Transaction signing and validation  

**To install:**
```bash
cd third_party/
git submodule add https://github.com/bitcoin-core/secp256k1.git
cd secp256k1
./autogen.sh
./configure --enable-module-schnorrsig --enable-module-extrakeys
make
```

### 2. LevelDB (HIGH PRIORITY)
**Purpose:** Blockchain storage and indexing  
**Status:** ⚠️ Submodule placeholder only  
**Required for:** Block persistence, UTXO set storage  

**To install:**
```bash
cd third_party/
git submodule add https://github.com/google/leveldb.git
cd leveldb
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
```

### 3. cpp-httplib (✅ COMPLETE)
**Purpose:** HTTP server for RPC  
**Status:** ✅ Already integrated (header-only)  
**Location:** `third_party/cpp-httplib/`

### 4. nlohmann/json (✅ COMPLETE)
**Purpose:** JSON parsing and serialization  
**Status:** ✅ Already integrated (header-only)  
**Location:** `third_party/json/`

## Quick Setup (All Dependencies)

```bash
cd /path/to/PantheonChain

# Initialize and update git submodules
git submodule update --init --recursive

# Build secp256k1
cd third_party/secp256k1
./autogen.sh
./configure --enable-module-schnorrsig --enable-module-extrakeys
make
cd ../..

# LevelDB will be built automatically by CMake
# cpp-httplib and json are header-only (no build needed)

# Now build PantheonChain
mkdir -p build && cd build
cmake .. -G Ninja
ninja
```

## Troubleshooting

### Error: secp256k1.h not found
- Ensure secp256k1 submodule is cloned and built
- Check that `third_party/secp256k1/include/` exists

### Error: leveldb CMakeLists.txt not found
- Ensure leveldb submodule is cloned
- Run `git submodule update --init --recursive`

## Notes for Developers

- The placeholder CMakeLists.txt files in empty submodule directories are stubs to allow partial builds
- For production deployment, all dependencies MUST be properly initialized
- Header-only libraries (cpp-httplib, json) work without additional setup
