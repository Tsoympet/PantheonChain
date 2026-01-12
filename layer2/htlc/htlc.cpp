#include "htlc.h"

namespace pantheon {
namespace layer2 {

HTLC::HTLC(const std::vector<uint8_t>& hash_lock,
           uint32_t time_lock,
           uint64_t amount,
           const std::vector<uint8_t>& sender,
           const std::vector<uint8_t>& receiver)
    : hash_lock_(hash_lock),
      time_lock_(time_lock),
      amount_(amount),
      sender_(sender),
      receiver_(receiver),
      claimed_(false) {
}

bool HTLC::ClaimWithPreimage(const std::vector<uint8_t>& preimage) {
    if (claimed_) {
        return false;
    }
    
    if (!VerifyPreimage(preimage)) {
        return false;
    }
    
    claimed_ = true;
    return true;
}

bool HTLC::ClaimWithTimeout(uint32_t current_time) {
    if (claimed_) {
        return false;
    }
    
    if (current_time < time_lock_) {
        return false;
    }
    
    claimed_ = true;
    return true;
}

bool HTLC::IsExpired(uint32_t current_time) const {
    return current_time >= time_lock_;
}

bool HTLC::VerifyPreimage(const std::vector<uint8_t>& preimage) const {
    auto hash_arr = pantheon::crypto::SHA256::Hash256(preimage);
    std::vector<uint8_t> hash(hash_arr.begin(), hash_arr.end());
    return hash == hash_lock_;
}

HTLCRoute::HTLCRoute(const std::vector<uint8_t>& payment_hash,
                     uint64_t total_amount)
    : payment_hash_(payment_hash),
      total_amount_(total_amount) {
}

void HTLCRoute::AddHop(const RouteHop& hop) {
    hops_.push_back(hop);
}

uint64_t HTLCRoute::GetTotalFees() const {
    uint64_t total_fees = 0;
    for (const auto& hop : hops_) {
        total_fees += hop.fee;
    }
    return total_fees;
}

bool HTLCRoute::Validate() const {
    if (hops_.empty()) {
        return false;
    }
    
    // Check CLTV expiry decreases along route
    for (size_t i = 1; i < hops_.size(); ++i) {
        if (hops_[i].cltv_expiry >= hops_[i-1].cltv_expiry) {
            return false;
        }
    }
    
    return true;
}

} // namespace layer2
} // namespace pantheon
