#include "payment_channel.h"

#include <cstring>
#include <ctime>

namespace parthenon {
namespace layer2 {

PaymentChannel::PaymentChannel(const std::vector<uint8_t>& party_a_pubkey,
                               const std::vector<uint8_t>& party_b_pubkey,
                               const ChannelBalance& initial_balance_a,
                               const ChannelBalance& initial_balance_b)
    : party_a_pubkey_(party_a_pubkey),
      party_b_pubkey_(party_b_pubkey),
      balance_a_(initial_balance_a),
      balance_b_(initial_balance_b),
      initial_balance_a_(initial_balance_a),
      initial_balance_b_(initial_balance_b),
      sequence_(0),
      state_(ChannelState::FUNDING),
      close_initiated_time_(0),
      dispute_period_(0) {
    // Generate channel ID from pubkeys
    std::vector<uint8_t> data;
    data.insert(data.end(), party_a_pubkey.begin(), party_a_pubkey.end());
    data.insert(data.end(), party_b_pubkey.begin(), party_b_pubkey.end());

    auto hash_arr = parthenon::crypto::SHA256::Hash256(data);
    channel_id_ = std::vector<uint8_t>(hash_arr.begin(), hash_arr.end());
}

bool PaymentChannel::Open() {
    if (state_ != ChannelState::FUNDING) {
        return false;
    }

    state_ = ChannelState::OPEN;
    return true;
}

bool PaymentChannel::UpdateState(const ChannelBalance& new_balance_a,
                                 const ChannelBalance& new_balance_b, uint64_t new_sequence,
                                 const std::vector<uint8_t>& signature_a,
                                 const std::vector<uint8_t>& signature_b) {
    if (state_ != ChannelState::OPEN) {
        return false;
    }

    // Sequence must increase
    if (new_sequence <= sequence_) {
        return false;
    }

    // Verify balances don't exceed initial deposits
    if (new_balance_a.taln + new_balance_b.taln !=
        initial_balance_a_.taln + initial_balance_b_.taln) {
        return false;
    }
    if (new_balance_a.drm + new_balance_b.drm != initial_balance_a_.drm + initial_balance_b_.drm) {
        return false;
    }
    if (new_balance_a.obl + new_balance_b.obl != initial_balance_a_.obl + initial_balance_b_.obl) {
        return false;
    }

    // In production, verify signatures here
    if (signature_a.empty() || signature_b.empty()) {
        return false;
    }

    balance_a_ = new_balance_a;
    balance_b_ = new_balance_b;
    sequence_ = new_sequence;

    return true;
}

bool PaymentChannel::InitiateClose(uint32_t dispute_period) {
    if (state_ != ChannelState::OPEN) {
        return false;
    }

    state_ = ChannelState::CLOSING;
    close_initiated_time_ = static_cast<uint32_t>(std::time(nullptr));
    dispute_period_ = dispute_period;

    return true;
}

bool PaymentChannel::FinalizeClose() {
    if (state_ != ChannelState::CLOSING) {
        return false;
    }

    // Check if dispute period has elapsed
    uint32_t current_time = static_cast<uint32_t>(std::time(nullptr));
    if (current_time < close_initiated_time_ + dispute_period_) {
        return false;
    }

    state_ = ChannelState::CLOSED;
    return true;
}

bool PaymentChannel::VerifyBalances() const {
    // Ensure conservation of funds
    if (balance_a_.taln + balance_b_.taln != initial_balance_a_.taln + initial_balance_b_.taln) {
        return false;
    }
    if (balance_a_.drm + balance_b_.drm != initial_balance_a_.drm + initial_balance_b_.drm) {
        return false;
    }
    if (balance_a_.obl + balance_b_.obl != initial_balance_a_.obl + initial_balance_b_.obl) {
        return false;
    }

    return true;
}

}  // namespace layer2
}  // namespace parthenon
