#ifndef PARTHENON_SETTLEMENT_ESCROW_H
#define PARTHENON_SETTLEMENT_ESCROW_H

#include <cstdint>
#include <vector>
#include <array>

namespace parthenon {
namespace settlement {

// Escrow types
enum class EscrowType : uint8_t {
    TIME_LOCKED = 0,      // Release after specific timestamp
    HASH_LOCKED = 1,       // Release with preimage reveal
    CONDITIONAL = 2        // Release based on condition
};

// Hash-locked escrow preimage
using Preimage = std::array<uint8_t, 32>;
using Hash256 = std::array<uint8_t, 32>;

// Time-locked escrow: funds released after locktime
class TimeLockEscrow {
public:
    TimeLockEscrow();
    explicit TimeLockEscrow(uint64_t locktime);
    
    uint64_t GetLocktime() const { return locktime_; }
    bool IsReleasable(uint64_t current_time) const;
    
    std::vector<uint8_t> Serialize() const;
    static TimeLockEscrow Deserialize(const std::vector<uint8_t>& data, size_t& pos);

private:
    uint64_t locktime_;  // Unix timestamp
};

// Hash-locked escrow: funds released when preimage revealed
class HashLockEscrow {
public:
    HashLockEscrow();
    explicit HashLockEscrow(const Hash256& hash);
    
    const Hash256& GetHash() const { return hash_; }
    bool VerifyPreimage(const Preimage& preimage) const;
    
    std::vector<uint8_t> Serialize() const;
    static HashLockEscrow Deserialize(const std::vector<uint8_t>& data, size_t& pos);

private:
    Hash256 hash_;
};

// Conditional escrow: combines time and hash locks
class ConditionalEscrow {
public:
    ConditionalEscrow();
    ConditionalEscrow(uint64_t locktime, const Hash256& hash);
    
    uint64_t GetLocktime() const { return locktime_; }
    const Hash256& GetHash() const { return hash_; }
    
    bool IsReleasable(uint64_t current_time, const Preimage* preimage) const;
    
    std::vector<uint8_t> Serialize() const;
    static ConditionalEscrow Deserialize(const std::vector<uint8_t>& data, size_t& pos);

private:
    uint64_t locktime_;
    Hash256 hash_;
};

// Escrow container
class Escrow {
public:
    Escrow();
    explicit Escrow(EscrowType type);
    
    EscrowType GetType() const { return type_; }
    
    void SetTimeLock(const TimeLockEscrow& timelock);
    void SetHashLock(const HashLockEscrow& hashlock);
    void SetConditional(const ConditionalEscrow& conditional);
    
    const TimeLockEscrow* GetTimeLock() const;
    const HashLockEscrow* GetHashLock() const;
    const ConditionalEscrow* GetConditional() const;
    
    bool IsReleasable(uint64_t current_time, const Preimage* preimage = nullptr) const;
    
    std::vector<uint8_t> Serialize() const;
    static Escrow Deserialize(const std::vector<uint8_t>& data, size_t& pos);

private:
    EscrowType type_;
    TimeLockEscrow timelock_;
    HashLockEscrow hashlock_;
    ConditionalEscrow conditional_;
};

} // namespace settlement
} // namespace parthenon

#endif // PARTHENON_SETTLEMENT_ESCROW_H
