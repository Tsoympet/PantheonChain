#include "escrow.h"
#include "../core/crypto/sha256.h"
#include <cstring>

namespace parthenon {
namespace settlement {

// TimeLockEscrow implementation
TimeLockEscrow::TimeLockEscrow() : locktime_(0) {}

TimeLockEscrow::TimeLockEscrow(uint64_t locktime) : locktime_(locktime) {}

bool TimeLockEscrow::IsReleasable(uint64_t current_time) const {
    return current_time >= locktime_;
}

std::vector<uint8_t> TimeLockEscrow::Serialize() const {
    std::vector<uint8_t> result(8);
    for (int i = 0; i < 8; ++i) {
        result[i] = static_cast<uint8_t>((locktime_ >> (i * 8)) & 0xFF);
    }
    return result;
}

TimeLockEscrow TimeLockEscrow::Deserialize(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos + 8 > data.size()) {
        return TimeLockEscrow();
    }
    
    uint64_t locktime = 0;
    for (int i = 0; i < 8; ++i) {
        locktime |= (static_cast<uint64_t>(data[pos + i]) << (i * 8));
    }
    pos += 8;
    
    return TimeLockEscrow(locktime);
}

// HashLockEscrow implementation
HashLockEscrow::HashLockEscrow() : hash_() {
    hash_.fill(0);
}

HashLockEscrow::HashLockEscrow(const Hash256& hash) : hash_(hash) {}

bool HashLockEscrow::VerifyPreimage(const Preimage& preimage) const {
    // Hash the preimage using SHA-256
    auto computed_hash = crypto::SHA256::Hash256(preimage.data(), preimage.size());
    
    // Compare with stored hash
    return computed_hash == hash_;
}

std::vector<uint8_t> HashLockEscrow::Serialize() const {
    return std::vector<uint8_t>(hash_.begin(), hash_.end());
}

HashLockEscrow HashLockEscrow::Deserialize(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos + 32 > data.size()) {
        return HashLockEscrow();
    }
    
    Hash256 hash;
    std::copy(data.begin() + pos, data.begin() + pos + 32, hash.begin());
    pos += 32;
    
    return HashLockEscrow(hash);
}

// ConditionalEscrow implementation
ConditionalEscrow::ConditionalEscrow() : locktime_(0), hash_() {
    hash_.fill(0);
}

ConditionalEscrow::ConditionalEscrow(uint64_t locktime, const Hash256& hash)
    : locktime_(locktime), hash_(hash) {}

bool ConditionalEscrow::IsReleasable(uint64_t current_time, const Preimage* preimage) const {
    // Must meet time requirement
    if (current_time < locktime_) {
        return false;
    }
    
    // Must provide valid preimage
    if (preimage == nullptr) {
        return false;
    }
    
    // Verify preimage
    auto computed_hash = crypto::SHA256::Hash256(preimage->data(), preimage->size());
    
    return computed_hash == hash_;
}

std::vector<uint8_t> ConditionalEscrow::Serialize() const {
    std::vector<uint8_t> result;
    
    // Serialize locktime
    for (int i = 0; i < 8; ++i) {
        result.push_back(static_cast<uint8_t>((locktime_ >> (i * 8)) & 0xFF));
    }
    
    // Serialize hash
    result.insert(result.end(), hash_.begin(), hash_.end());
    
    return result;
}

ConditionalEscrow ConditionalEscrow::Deserialize(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos + 40 > data.size()) {
        return ConditionalEscrow();
    }
    
    // Deserialize locktime
    uint64_t locktime = 0;
    for (int i = 0; i < 8; ++i) {
        locktime |= (static_cast<uint64_t>(data[pos + i]) << (i * 8));
    }
    pos += 8;
    
    // Deserialize hash
    Hash256 hash;
    std::copy(data.begin() + pos, data.begin() + pos + 32, hash.begin());
    pos += 32;
    
    return ConditionalEscrow(locktime, hash);
}

// Escrow implementation
Escrow::Escrow() : type_(EscrowType::TIME_LOCKED), timelock_(), hashlock_(), conditional_() {}

Escrow::Escrow(EscrowType type) : type_(type), timelock_(), hashlock_(), conditional_() {}

void Escrow::SetTimeLock(const TimeLockEscrow& timelock) {
    type_ = EscrowType::TIME_LOCKED;
    timelock_ = timelock;
}

void Escrow::SetHashLock(const HashLockEscrow& hashlock) {
    type_ = EscrowType::HASH_LOCKED;
    hashlock_ = hashlock;
}

void Escrow::SetConditional(const ConditionalEscrow& conditional) {
    type_ = EscrowType::CONDITIONAL;
    conditional_ = conditional;
}

const TimeLockEscrow* Escrow::GetTimeLock() const {
    return (type_ == EscrowType::TIME_LOCKED) ? &timelock_ : nullptr;
}

const HashLockEscrow* Escrow::GetHashLock() const {
    return (type_ == EscrowType::HASH_LOCKED) ? &hashlock_ : nullptr;
}

const ConditionalEscrow* Escrow::GetConditional() const {
    return (type_ == EscrowType::CONDITIONAL) ? &conditional_ : nullptr;
}

bool Escrow::IsReleasable(uint64_t current_time, const Preimage* preimage) const {
    switch (type_) {
        case EscrowType::TIME_LOCKED:
            return timelock_.IsReleasable(current_time);
        case EscrowType::HASH_LOCKED:
            return preimage != nullptr && hashlock_.VerifyPreimage(*preimage);
        case EscrowType::CONDITIONAL:
            return conditional_.IsReleasable(current_time, preimage);
        default:
            return false;
    }
}

std::vector<uint8_t> Escrow::Serialize() const {
    std::vector<uint8_t> result;
    
    // Serialize type
    result.push_back(static_cast<uint8_t>(type_));
    
    // Serialize escrow data based on type
    switch (type_) {
        case EscrowType::TIME_LOCKED: {
            auto data = timelock_.Serialize();
            result.insert(result.end(), data.begin(), data.end());
            break;
        }
        case EscrowType::HASH_LOCKED: {
            auto data = hashlock_.Serialize();
            result.insert(result.end(), data.begin(), data.end());
            break;
        }
        case EscrowType::CONDITIONAL: {
            auto data = conditional_.Serialize();
            result.insert(result.end(), data.begin(), data.end());
            break;
        }
    }
    
    return result;
}

Escrow Escrow::Deserialize(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos + 1 > data.size()) {
        return Escrow();
    }
    
    // Deserialize type
    EscrowType type = static_cast<EscrowType>(data[pos]);
    pos++;
    
    Escrow escrow(type);
    
    // Deserialize escrow data based on type
    switch (type) {
        case EscrowType::TIME_LOCKED: {
            auto timelock = TimeLockEscrow::Deserialize(data, pos);
            escrow.SetTimeLock(timelock);
            break;
        }
        case EscrowType::HASH_LOCKED: {
            auto hashlock = HashLockEscrow::Deserialize(data, pos);
            escrow.SetHashLock(hashlock);
            break;
        }
        case EscrowType::CONDITIONAL: {
            auto conditional = ConditionalEscrow::Deserialize(data, pos);
            escrow.SetConditional(conditional);
            break;
        }
    }
    
    return escrow;
}

} // namespace settlement
} // namespace parthenon
