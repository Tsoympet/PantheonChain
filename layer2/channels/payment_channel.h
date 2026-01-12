#ifndef PANTHEON_LAYER2_CHANNELS_PAYMENT_CHANNEL_H
#define PANTHEON_LAYER2_CHANNELS_PAYMENT_CHANNEL_H

#include "layer1/core/primitives/transaction.h"
#include "layer1/core/crypto/sha256.h"
#include <cstdint>
#include <vector>
#include <map>

namespace parthenon {
namespace layer2 {

enum class ChannelState {
    FUNDING,    // Channel being funded
    OPEN,       // Channel active
    CLOSING,    // Dispute period active
    CLOSED      // Channel finalized
};

struct ChannelBalance {
    uint64_t taln;
    uint64_t drm;
    uint64_t obl;
    
    ChannelBalance() : taln(0), drm(0), obl(0) {}
    ChannelBalance(uint64_t t, uint64_t d, uint64_t o) 
        : taln(t), drm(d), obl(o) {}
};

class PaymentChannel {
public:
    PaymentChannel(const std::vector<uint8_t>& party_a_pubkey,
                   const std::vector<uint8_t>& party_b_pubkey,
                   const ChannelBalance& initial_balance_a,
                   const ChannelBalance& initial_balance_b);
    
    std::vector<uint8_t> GetChannelId() const { return channel_id_; }
    
    ChannelState GetState() const { return state_; }
    
    bool Open();
    
    bool UpdateState(const ChannelBalance& new_balance_a,
                     const ChannelBalance& new_balance_b,
                     uint64_t new_sequence,
                     const std::vector<uint8_t>& signature_a,
                     const std::vector<uint8_t>& signature_b);
    
    bool InitiateClose(uint32_t dispute_period);
    
    bool FinalizeClose();
    
    ChannelBalance GetBalanceA() const { return balance_a_; }
    ChannelBalance GetBalanceB() const { return balance_b_; }
    uint64_t GetSequence() const { return sequence_; }
    
    bool VerifyBalances() const;
    
private:
    std::vector<uint8_t> channel_id_;
    std::vector<uint8_t> party_a_pubkey_;
    std::vector<uint8_t> party_b_pubkey_;
    ChannelBalance balance_a_;
    ChannelBalance balance_b_;
    ChannelBalance initial_balance_a_;
    ChannelBalance initial_balance_b_;
    uint64_t sequence_;
    ChannelState state_;
    uint32_t close_initiated_time_;
    uint32_t dispute_period_;
};

} // namespace layer2
} // namespace parthenon

#endif // PANTHEON_LAYER2_CHANNELS_PAYMENT_CHANNEL_H
