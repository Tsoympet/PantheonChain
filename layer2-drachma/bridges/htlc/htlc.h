#ifndef PANTHEON_LAYER2_HTLC_HTLC_H
#define PANTHEON_LAYER2_HTLC_HTLC_H

#include "layer1/core/crypto/sha256.h"

#include <cstdint>
#include <vector>

namespace parthenon {
namespace layer2 {

class HTLC {
  public:
    HTLC(const std::vector<uint8_t>& hash_lock, uint32_t time_lock, uint64_t amount,
         const std::vector<uint8_t>& sender, const std::vector<uint8_t>& receiver);

    std::vector<uint8_t> GetHashLock() const { return hash_lock_; }
    uint32_t GetTimeLock() const { return time_lock_; }
    uint64_t GetAmount() const { return amount_; }

    bool ClaimWithPreimage(const std::vector<uint8_t>& preimage);

    bool ClaimWithTimeout(uint32_t current_time);

    bool IsExpired(uint32_t current_time) const;

    bool VerifyPreimage(const std::vector<uint8_t>& preimage) const;

  private:
    std::vector<uint8_t> hash_lock_;
    uint32_t time_lock_;
    uint64_t amount_;
    std::vector<uint8_t> sender_;
    std::vector<uint8_t> receiver_;
    bool claimed_;
};

struct RouteHop {
    std::vector<uint8_t> node_pubkey;
    uint64_t fee;
    uint32_t cltv_expiry;

    RouteHop(const std::vector<uint8_t>& pubkey, uint64_t f, uint32_t expiry)
        : node_pubkey(pubkey), fee(f), cltv_expiry(expiry) {}
};

class HTLCRoute {
  public:
    HTLCRoute(const std::vector<uint8_t>& payment_hash, uint64_t total_amount);

    void AddHop(const RouteHop& hop);

    std::vector<RouteHop> GetHops() const { return hops_; }

    uint64_t GetTotalAmount() const { return total_amount_; }
    uint64_t GetTotalFees() const;

    std::vector<uint8_t> GetPaymentHash() const { return payment_hash_; }

    bool Validate() const;

  private:
    std::vector<uint8_t> payment_hash_;
    uint64_t total_amount_;
    std::vector<RouteHop> hops_;
};

}  // namespace layer2
}  // namespace parthenon

#endif  // PANTHEON_LAYER2_HTLC_HTLC_H
