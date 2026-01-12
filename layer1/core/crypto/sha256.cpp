// ParthenonChain - SHA-256 Implementation
// Based on FIPS 180-4 specification
// Consensus-critical: DO NOT MODIFY without network-wide coordination

#include "sha256.h"
#include <algorithm>

namespace parthenon {
namespace crypto {

// SHA-256 constants (first 32 bits of fractional parts of cube roots of first 64 primes)
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Initial hash values (first 32 bits of fractional parts of square roots of first 8 primes)
static const uint32_t H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

// Bitwise operations
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SIGMA0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define SIGMA1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define sigma0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define sigma1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static inline uint32_t ReadBE32(const uint8_t* ptr) {
    return ((uint32_t)ptr[0] << 24) |
           ((uint32_t)ptr[1] << 16) |
           ((uint32_t)ptr[2] << 8) |
           ((uint32_t)ptr[3]);
}

static inline void WriteBE32(uint8_t* ptr, uint32_t val) {
    ptr[0] = val >> 24;
    ptr[1] = val >> 16;
    ptr[2] = val >> 8;
    ptr[3] = val;
}

static inline void WriteBE64(uint8_t* ptr, uint64_t val) {
    ptr[0] = val >> 56;
    ptr[1] = val >> 48;
    ptr[2] = val >> 40;
    ptr[3] = val >> 32;
    ptr[4] = val >> 24;
    ptr[5] = val >> 16;
    ptr[6] = val >> 8;
    ptr[7] = val;
}

SHA256::SHA256() {
    Reset();
}

SHA256::~SHA256() {
    // Clear sensitive data
    std::fill_n(state_, 8, 0);
    std::fill_n(buffer_, BLOCK_SIZE, 0);
}

void SHA256::Reset() {
    std::copy(H0, H0 + 8, state_);
    byte_count_ = 0;
    buffer_size_ = 0;
}

void SHA256::Transform(const uint8_t* chunk) {
    uint32_t W[64];
    
    // Prepare message schedule
    for (int t = 0; t < 16; ++t) {
        W[t] = ReadBE32(chunk + t * 4);
    }
    
    for (int t = 16; t < 64; ++t) {
        W[t] = sigma1(W[t - 2]) + W[t - 7] + sigma0(W[t - 15]) + W[t - 16];
    }
    
    // Initialize working variables
    uint32_t a = state_[0];
    uint32_t b = state_[1];
    uint32_t c = state_[2];
    uint32_t d = state_[3];
    uint32_t e = state_[4];
    uint32_t f = state_[5];
    uint32_t g = state_[6];
    uint32_t h = state_[7];
    
    // Main loop
    for (int t = 0; t < 64; ++t) {
        uint32_t T1 = h + SIGMA1(e) + CH(e, f, g) + K[t] + W[t];
        uint32_t T2 = SIGMA0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }
    
    // Update state
    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
}

void SHA256::Write(const uint8_t* data, size_t len) {
    const uint8_t* ptr = data;
    size_t remaining = len;
    
    while (remaining > 0) {
        size_t to_copy = std::min(remaining, BLOCK_SIZE - buffer_size_);
        std::copy(ptr, ptr + to_copy, buffer_ + buffer_size_);
        buffer_size_ += to_copy;
        ptr += to_copy;
        remaining -= to_copy;
        byte_count_ += to_copy;
        
        if (buffer_size_ == BLOCK_SIZE) {
            Transform(buffer_);
            buffer_size_ = 0;
        }
    }
}

void SHA256::Write(const std::vector<uint8_t>& data) {
    Write(data.data(), data.size());
}

SHA256::Hash SHA256::Finalize() {
    // Padding
    uint8_t padding[BLOCK_SIZE * 2] = {0x80}; // First padding byte
    size_t padding_len = (buffer_size_ < 56) ? (56 - buffer_size_) : (120 - buffer_size_);
    
    // Append length in bits as 64-bit big-endian
    WriteBE64(padding + padding_len, byte_count_ * 8);
    Write(padding, padding_len + 8);
    
    // Extract hash
    Hash result;
    for (int i = 0; i < 8; ++i) {
        WriteBE32(result.data() + i * 4, state_[i]);
    }
    
    // Reset for potential reuse
    Reset();
    
    return result;
}

SHA256::Hash SHA256::Hash256(const uint8_t* data, size_t len) {
    SHA256 hasher;
    hasher.Write(data, len);
    return hasher.Finalize();
}

SHA256::Hash SHA256::Hash256(const std::vector<uint8_t>& data) {
    return Hash256(data.data(), data.size());
}

// SHA256d implementation
SHA256d::Hash SHA256d::Hash256d(const uint8_t* data, size_t len) {
    auto first_hash = SHA256::Hash256(data, len);
    return SHA256::Hash256(first_hash.data(), first_hash.size());
}

SHA256d::Hash SHA256d::Hash256d(const std::vector<uint8_t>& data) {
    return Hash256d(data.data(), data.size());
}

// Tagged SHA-256 implementation (BIP-340 style)
TaggedSHA256::TaggedSHA256(const std::string& tag) {
    auto tag_hash = SHA256::Hash256(reinterpret_cast<const uint8_t*>(tag.data()), tag.size());
    hasher_.Write(tag_hash.data(), tag_hash.size());
    hasher_.Write(tag_hash.data(), tag_hash.size());
}

void TaggedSHA256::Write(const uint8_t* data, size_t len) {
    hasher_.Write(data, len);
}

void TaggedSHA256::Write(const std::vector<uint8_t>& data) {
    hasher_.Write(data);
}

TaggedSHA256::Hash TaggedSHA256::Finalize() {
    return hasher_.Finalize();
}

TaggedSHA256::Hash TaggedSHA256::HashTagged(const std::string& tag, const uint8_t* data, size_t len) {
    TaggedSHA256 hasher(tag);
    hasher.Write(data, len);
    return hasher.Finalize();
}

TaggedSHA256::Hash TaggedSHA256::HashTagged(const std::string& tag, const std::vector<uint8_t>& data) {
    return HashTagged(tag, data.data(), data.size());
}

#undef ROTR
#undef CH
#undef MAJ
#undef SIGMA0
#undef SIGMA1
#undef sigma0
#undef sigma1

} // namespace crypto
} // namespace parthenon
