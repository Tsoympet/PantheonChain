// ParthenonChain - EIP-1559 Gas Pricing
// Dynamic fee market with base fee and priority fee

#pragma once

#include <cstdint>
#include <algorithm>

namespace parthenon {
namespace evm {

/**
 * EIP-1559 gas pricing parameters
 */
struct GasPricing {
    static constexpr uint64_t TARGET_GAS_PER_BLOCK = 15000000;  // 15M gas target
    static constexpr uint64_t MAX_GAS_PER_BLOCK = 30000000;     // 30M gas limit
    static constexpr uint64_t BASE_FEE_CHANGE_DENOMINATOR = 8;  // 12.5% max change
    static constexpr uint64_t ELASTICITY_MULTIPLIER = 2;        // Block can be 2x target
    static constexpr uint64_t INITIAL_BASE_FEE = 1000000000;    // 1 Gwei
    static constexpr uint64_t MIN_BASE_FEE = 7;                 // Minimum base fee (wei)
};

/**
 * Calculate next block's base fee using EIP-1559 formula
 * 
 * @param parent_base_fee Previous block's base fee
 * @param parent_gas_used Gas used in previous block
 * @param parent_gas_limit Gas limit of previous block
 * @return Next block's base fee
 */
inline uint64_t CalculateNextBaseFee(
    uint64_t parent_base_fee,
    uint64_t parent_gas_used,
    uint64_t parent_gas_limit
) {
    // If parent block is empty, decrease base fee
    if (parent_gas_used == 0) {
        uint64_t decrease = parent_base_fee / GasPricing::BASE_FEE_CHANGE_DENOMINATOR;
        if (parent_base_fee > decrease) {
            return std::max(parent_base_fee - decrease, GasPricing::MIN_BASE_FEE);
        }
        return GasPricing::MIN_BASE_FEE;
    }
    
    uint64_t target_gas = parent_gas_limit / GasPricing::ELASTICITY_MULTIPLIER;
    
    // If gas used equals target, base fee stays the same
    if (parent_gas_used == target_gas) {
        return parent_base_fee;
    }
    
    // If gas used is above target, increase base fee
    if (parent_gas_used > target_gas) {
        uint64_t gas_delta = parent_gas_used - target_gas;
        
        // Calculate fee increase
        // base_fee_delta = parent_base_fee * gas_delta / target_gas / BASE_FEE_CHANGE_DENOMINATOR
        
        // Prevent overflow by doing multiplication and division carefully
        uint64_t base_fee_delta;
        if (parent_base_fee <= UINT64_MAX / gas_delta) {
            base_fee_delta = (parent_base_fee * gas_delta) / target_gas / GasPricing::BASE_FEE_CHANGE_DENOMINATOR;
        } else {
            // Do division first to prevent overflow
            base_fee_delta = (parent_base_fee / GasPricing::BASE_FEE_CHANGE_DENOMINATOR) * gas_delta / target_gas;
        }
        
        // Ensure at least +1 increase
        base_fee_delta = std::max(base_fee_delta, static_cast<uint64_t>(1));
        
        // Check for overflow
        if (parent_base_fee <= UINT64_MAX - base_fee_delta) {
            return parent_base_fee + base_fee_delta;
        }
        return UINT64_MAX; // Cap at maximum
    }
    
    // If gas used is below target, decrease base fee
    uint64_t gas_delta = target_gas - parent_gas_used;
    
    // Calculate fee decrease
    uint64_t base_fee_delta;
    if (parent_base_fee <= UINT64_MAX / gas_delta) {
        base_fee_delta = (parent_base_fee * gas_delta) / target_gas / GasPricing::BASE_FEE_CHANGE_DENOMINATOR;
    } else {
        base_fee_delta = (parent_base_fee / GasPricing::BASE_FEE_CHANGE_DENOMINATOR) * gas_delta / target_gas;
    }
    
    if (parent_base_fee > base_fee_delta) {
        return std::max(parent_base_fee - base_fee_delta, GasPricing::MIN_BASE_FEE);
    }
    
    return GasPricing::MIN_BASE_FEE;
}

/**
 * Calculate effective gas price for a transaction (EIP-1559)
 * 
 * @param base_fee Current block's base fee
 * @param max_fee_per_gas Maximum fee per gas user is willing to pay
 * @param max_priority_fee_per_gas Maximum priority fee (tip) per gas
 * @return Effective gas price paid by transaction
 */
inline uint64_t CalculateEffectiveGasPrice(
    uint64_t base_fee,
    uint64_t max_fee_per_gas,
    uint64_t max_priority_fee_per_gas
) {
    // Effective priority fee is the minimum of:
    // 1. max_priority_fee_per_gas
    // 2. max_fee_per_gas - base_fee
    uint64_t effective_priority_fee;
    
    if (max_fee_per_gas >= base_fee) {
        uint64_t max_affordable_priority = max_fee_per_gas - base_fee;
        effective_priority_fee = std::min(max_priority_fee_per_gas, max_affordable_priority);
    } else {
        // Transaction underpays base fee, should be rejected
        effective_priority_fee = 0;
    }
    
    return base_fee + effective_priority_fee;
}

/**
 * Validate EIP-1559 transaction fees
 * 
 * @param base_fee Current block's base fee
 * @param max_fee_per_gas Transaction's max fee per gas
 * @param max_priority_fee_per_gas Transaction's max priority fee per gas
 * @return true if transaction fees are valid
 */
inline bool ValidateTransactionFees(
    uint64_t base_fee,
    uint64_t max_fee_per_gas,
    uint64_t max_priority_fee_per_gas
) {
    // max_fee_per_gas must be at least base_fee
    if (max_fee_per_gas < base_fee) {
        return false;
    }
    
    // max_priority_fee_per_gas cannot exceed max_fee_per_gas
    if (max_priority_fee_per_gas > max_fee_per_gas) {
        return false;
    }
    
    return true;
}

/**
 * Calculate total transaction fee (gas used * effective gas price)
 * 
 * @param gas_used Amount of gas used by transaction
 * @param base_fee Current block's base fee
 * @param max_fee_per_gas Transaction's max fee per gas
 * @param max_priority_fee_per_gas Transaction's max priority fee per gas
 * @param base_fee_burned Output: amount of base fee burned
 * @param priority_fee_paid Output: amount of priority fee paid to miner
 * @return Total fee paid (base_fee_burned + priority_fee_paid)
 */
inline uint64_t CalculateTransactionFee(
    uint64_t gas_used,
    uint64_t base_fee,
    uint64_t max_fee_per_gas,
    uint64_t max_priority_fee_per_gas,
    uint64_t& base_fee_burned,
    uint64_t& priority_fee_paid
) {
    uint64_t effective_gas_price = CalculateEffectiveGasPrice(
        base_fee, max_fee_per_gas, max_priority_fee_per_gas
    );
    
    // Base fee portion is burned
    base_fee_burned = gas_used * base_fee;
    
    // Priority fee goes to miner
    uint64_t priority_fee_per_gas = effective_gas_price >= base_fee ? 
        effective_gas_price - base_fee : 0;
    priority_fee_paid = gas_used * priority_fee_per_gas;
    
    return base_fee_burned + priority_fee_paid;
}

/**
 * Estimate gas price for next block based on current congestion
 * 
 * @param current_base_fee Current block's base fee
 * @param recommended_priority_fee Recommended priority fee for fast inclusion
 * @return Recommended max_fee_per_gas for transactions
 */
inline uint64_t EstimateGasPrice(
    uint64_t current_base_fee,
    uint64_t recommended_priority_fee = 1000000000  // 1 Gwei default tip
) {
    // Account for potential base fee increase in next block
    // Assume worst case: gas usage at max, causing ~12.5% increase
    uint64_t max_base_fee_increase = current_base_fee / GasPricing::BASE_FEE_CHANGE_DENOMINATOR;
    uint64_t estimated_next_base_fee = current_base_fee + max_base_fee_increase;
    
    // Add some buffer (10%) to be safe
    uint64_t buffer = estimated_next_base_fee / 10;
    estimated_next_base_fee += buffer;
    
    return estimated_next_base_fee + recommended_priority_fee;
}

} // namespace evm
} // namespace parthenon
