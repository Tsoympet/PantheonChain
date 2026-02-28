// ParthenonChain - Decentralized Exchange Implementation

#include "dex.h"

#include "crypto/sha256.h"
#include "primitives/safe_math.h"

#include <algorithm>
#include <ctime>

namespace parthenon {
namespace layer2 {
namespace dex {

using primitives::SafeMath;

// Static member initialization
std::map<std::vector<uint8_t>, LiquidityPool> AutomatedMarketMaker::pools_;

// OrderBook implementation
OrderBook::OrderBook(primitives::AssetID base, primitives::AssetID quote)
    : base_asset_(base), quote_asset_(quote) {}

std::vector<uint8_t> OrderBook::PlaceOrder(const Order& order) {
    if (!ValidateOrder(order)) {
        return {};
    }

    Order new_order = order;
    // Serialize order data safely instead of using reinterpret_cast on struct
    std::vector<uint8_t> order_data;
    order_data.reserve(sizeof(uint64_t) * 3 + order.trader_pubkey.size());
    for (int i = 0; i < 8; i++) order_data.push_back(static_cast<uint8_t>(order.amount >> (i * 8)));
    for (int i = 0; i < 8; i++) order_data.push_back(static_cast<uint8_t>(order.price >> (i * 8)));
    for (int i = 0; i < 8; i++) order_data.push_back(static_cast<uint8_t>(order.timestamp >> (i * 8)));
    order_data.insert(order_data.end(), order.trader_pubkey.begin(), order.trader_pubkey.end());
    
    auto hash = crypto::SHA256::Hash256(order_data);
    new_order.order_id = std::vector<uint8_t>(hash.begin(), hash.end());
    new_order.status = OrderStatus::PENDING;
    new_order.filled_amount = 0;
    new_order.timestamp = static_cast<uint64_t>(std::time(nullptr));

    orders_by_id_[new_order.order_id] = new_order;

    if (new_order.type == OrderType::LIMIT_BUY) {
        buy_orders_[new_order.price].push_back(new_order);
    } else if (new_order.type == OrderType::LIMIT_SELL) {
        sell_orders_[new_order.price].push_back(new_order);
    }

    return new_order.order_id;
}

bool OrderBook::CancelOrder(const std::vector<uint8_t>& order_id,
                            const std::vector<uint8_t>& trader_pubkey) {
    auto it = orders_by_id_.find(order_id);
    if (it == orders_by_id_.end()) {
        return false;
    }

    if (it->second.trader_pubkey != trader_pubkey) {
        return false;
    }

    it->second.status = OrderStatus::CANCELLED;
    RemoveOrder(order_id);
    return true;
}

std::vector<Trade> OrderBook::MatchOrders() {
    std::vector<Trade> trades;

    while (!buy_orders_.empty() && !sell_orders_.empty()) {
        auto best_bid_it = buy_orders_.rbegin();
        auto best_ask_it = sell_orders_.begin();

        if (best_bid_it->first < best_ask_it->first) {
            break;  // No overlap
        }

        auto& buy_order = best_bid_it->second.front();
        auto& sell_order = best_ask_it->second.front();

        uint64_t trade_amount = std::min(buy_order.amount - buy_order.filled_amount,
                                         sell_order.amount - sell_order.filled_amount);

        uint64_t trade_price = sell_order.price;

        Trade trade;
        // Serialize trade data safely instead of using reinterpret_cast on primitive
        std::vector<uint8_t> trade_data;
        trade_data.reserve(sizeof(uint64_t) * 2);
        for (int i = 0; i < 8; i++) trade_data.push_back(static_cast<uint8_t>(trade_amount >> (i * 8)));
        for (int i = 0; i < 8; i++) trade_data.push_back(static_cast<uint8_t>(trade_price >> (i * 8)));
        
        auto hash = crypto::SHA256::Hash256(trade_data);
        trade.trade_id = std::vector<uint8_t>(hash.begin(), hash.end());
        trade.buy_order_id = buy_order.order_id;
        trade.sell_order_id = sell_order.order_id;
        trade.base_asset = base_asset_;
        trade.quote_asset = quote_asset_;
        trade.price = trade_price;
        trade.amount = trade_amount;
        trade.timestamp = static_cast<uint64_t>(std::time(nullptr));
        trade.buyer_pubkey = buy_order.trader_pubkey;
        trade.seller_pubkey = sell_order.trader_pubkey;

        trades.push_back(trade);

        buy_order.filled_amount += trade_amount;
        sell_order.filled_amount += trade_amount;

        if (buy_order.filled_amount == buy_order.amount) {
            buy_order.status = OrderStatus::FILLED;
            best_bid_it->second.erase(best_bid_it->second.begin());
            if (best_bid_it->second.empty()) {
                buy_orders_.erase(std::prev(best_bid_it.base()));
            }
        }

        if (sell_order.filled_amount == sell_order.amount) {
            sell_order.status = OrderStatus::FILLED;
            best_ask_it->second.erase(best_ask_it->second.begin());
            if (best_ask_it->second.empty()) {
                sell_orders_.erase(best_ask_it);
            }
        }
    }

    return trades;
}

std::optional<Order> OrderBook::GetBestBid() const {
    if (buy_orders_.empty()) {
        return std::nullopt;
    }
    return buy_orders_.rbegin()->second.front();
}

std::optional<Order> OrderBook::GetBestAsk() const {
    if (sell_orders_.empty()) {
        return std::nullopt;
    }
    return sell_orders_.begin()->second.front();
}

OrderBook::MarketDepth OrderBook::GetDepth(size_t levels) const {
    MarketDepth depth;

    size_t count = 0;
    for (auto it = buy_orders_.rbegin(); it != buy_orders_.rend() && count < levels;
         ++it, ++count) {
        uint64_t total_amount = 0;
        for (const auto& order : it->second) {
            uint64_t unfilled = order.amount - order.filled_amount;
            // Check for overflow before adding
            if (total_amount > UINT64_MAX - unfilled) {
                total_amount = UINT64_MAX;  // Cap at maximum
                break;
            }
            total_amount += unfilled;
        }
        depth.bids.push_back({it->first, total_amount});
    }

    count = 0;
    for (auto it = sell_orders_.begin(); it != sell_orders_.end() && count < levels;
         ++it, ++count) {
        uint64_t total_amount = 0;
        for (const auto& order : it->second) {
            uint64_t unfilled = order.amount - order.filled_amount;
            // Check for overflow before adding
            if (total_amount > UINT64_MAX - unfilled) {
                total_amount = UINT64_MAX;  // Cap at maximum
                break;
            }
            total_amount += unfilled;
        }
        depth.asks.push_back({it->first, total_amount});
    }

    return depth;
}

std::optional<Order> OrderBook::GetOrder(const std::vector<uint8_t>& order_id) const {
    auto it = orders_by_id_.find(order_id);
    if (it == orders_by_id_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool OrderBook::ValidateOrder(const Order& order) const {
    // Basic validation
    if (order.amount == 0 || order.price == 0) {
        return false;
    }
    
    // Validate assets match this order book
    if (order.base_asset != base_asset_ || order.quote_asset != quote_asset_) {
        return false;
    }
    
    // Validate order type
    if (order.type != OrderType::LIMIT_BUY && order.type != OrderType::LIMIT_SELL &&
        order.type != OrderType::MARKET_BUY && order.type != OrderType::MARKET_SELL) {
        return false;
    }
    
    return true;
}

void OrderBook::RemoveOrder(const std::vector<uint8_t>& order_id) {
    orders_by_id_.erase(order_id);
}

// AutomatedMarketMaker implementation
std::vector<uint8_t> AutomatedMarketMaker::CreatePool(primitives::AssetID asset_a,
                                                      primitives::AssetID asset_b,
                                                      uint64_t initial_a, uint64_t initial_b,
                                                      uint64_t fee_rate) {
    LiquidityPool pool;
    pool.asset_a = asset_a;
    pool.asset_b = asset_b;
    pool.reserve_a = initial_a;
    pool.reserve_b = initial_b;
    
    // Check for integer overflow in multiplication
    if (initial_a > 0 && initial_b > UINT64_MAX / initial_a) {
        // Overflow would occur, use safe alternative
        pool.total_shares = UINT64_MAX;
    } else {
        pool.total_shares = initial_a * initial_b;
    }
    
    pool.fee_rate = fee_rate;

    std::vector<uint8_t> pool_data;
    pool_data.push_back(static_cast<uint8_t>(asset_a));
    pool_data.push_back(static_cast<uint8_t>(asset_b));
    auto hash = crypto::SHA256::Hash256(pool_data);
    pool.pool_id = std::vector<uint8_t>(hash.begin(), hash.end());

    pools_[pool.pool_id] = pool;
    return pool.pool_id;
}

uint64_t AutomatedMarketMaker::AddLiquidity(const std::vector<uint8_t>& pool_id, uint64_t amount_a,
                                            uint64_t amount_b,
                                            const std::vector<uint8_t>& provider_pubkey) {
    auto it = pools_.find(pool_id);
    if (it == pools_.end()) {
        return 0;
    }

    auto& pool = it->second;

    // Calculate proportional shares
    uint64_t shares = 0;
    if (pool.total_shares == 0) {
        // Check for overflow in multiplication
        if (amount_a > 0 && amount_b > UINT64_MAX / amount_a) {
            shares = UINT64_MAX;
        } else {
            shares = amount_a * amount_b;
        }
    } else {
        // Check for division by zero
        if (pool.reserve_a == 0 || pool.reserve_b == 0) {
            return 0;
        }

        uint64_t shares_a = 0;
        uint64_t shares_b = 0;

        // Safe calculation of shares_a (guard against overflow before multiply)
        if (pool.total_shares > 0 && amount_a > UINT64_MAX / pool.total_shares) {
            shares_a = UINT64_MAX;
        } else {
            shares_a = (amount_a * pool.total_shares) / pool.reserve_a;
        }

        // Safe calculation of shares_b (guard against overflow before multiply)
        if (pool.total_shares > 0 && amount_b > UINT64_MAX / pool.total_shares) {
            shares_b = UINT64_MAX;
        } else {
            shares_b = (amount_b * pool.total_shares) / pool.reserve_b;
        }

        shares = std::min(shares_a, shares_b);
    }
    
    // Check for overflow before adding to reserves and shares
    if (pool.reserve_a > UINT64_MAX - amount_a || 
        pool.reserve_b > UINT64_MAX - amount_b ||
        pool.total_shares > UINT64_MAX - shares) {
        return 0;  // Would overflow
    }

    pool.reserve_a += amount_a;
    pool.reserve_b += amount_b;
    pool.total_shares += shares;
    pool.shares[provider_pubkey] += shares;

    
    // Check for overflow in provider shares
    uint64_t current_shares = pool.shares[provider_pubkey];
    if (current_shares > UINT64_MAX - shares) {
        pool.shares[provider_pubkey] = UINT64_MAX;
    } else {
        pool.shares[provider_pubkey] += shares;
    }
    
    return shares;
}

std::pair<uint64_t, uint64_t>
AutomatedMarketMaker::RemoveLiquidity(const std::vector<uint8_t>& pool_id, uint64_t shares,
                                      const std::vector<uint8_t>& provider_pubkey) {
    auto it = pools_.find(pool_id);
    if (it == pools_.end()) {
        return {0, 0};
    }

    auto& pool = it->second;

    if (pool.shares[provider_pubkey] < shares) {
        return {0, 0};
    }

    
    // Check for division by zero
    if (pool.total_shares == 0) {
        return {0, 0};
    }
    
    uint64_t amount_a = (shares > UINT64_MAX / pool.reserve_a)
                            ? (pool.reserve_a / pool.total_shares * shares)
                            : (shares * pool.reserve_a) / pool.total_shares;
    uint64_t amount_b = (shares > UINT64_MAX / pool.reserve_b)
                            ? (pool.reserve_b / pool.total_shares * shares)
                            : (shares * pool.reserve_b) / pool.total_shares;

    pool.reserve_a -= amount_a;
    pool.reserve_b -= amount_b;
    pool.total_shares -= shares;
    pool.shares[provider_pubkey] -= shares;

    return {amount_a, amount_b};
}

uint64_t AutomatedMarketMaker::Swap(const std::vector<uint8_t>& pool_id,
                                    primitives::AssetID input_asset, uint64_t input_amount,
                                    uint64_t min_output_amount) {
    auto it = pools_.find(pool_id);
    if (it == pools_.end()) {
        return 0;
    }

    auto& pool = it->second;

    uint64_t input_reserve, output_reserve;
    if (input_asset == pool.asset_a) {
        input_reserve = pool.reserve_a;
        output_reserve = pool.reserve_b;
    } else {
        input_reserve = pool.reserve_b;
        output_reserve = pool.reserve_a;
    }

    uint64_t output_amount =
        GetOutputAmount(input_amount, input_reserve, output_reserve, pool.fee_rate);

    if (output_amount < min_output_amount) {
        return 0;
    }

    if (input_asset == pool.asset_a) {
        pool.reserve_a += input_amount;
        pool.reserve_b -= output_amount;
    } else {
        pool.reserve_b += input_amount;
        pool.reserve_a -= output_amount;
    }

    return output_amount;
}

std::optional<LiquidityPool> AutomatedMarketMaker::GetPool(const std::vector<uint8_t>& pool_id) {
    auto it = pools_.find(pool_id);
    if (it == pools_.end()) {
        return std::nullopt;
    }
    return it->second;
}

uint64_t AutomatedMarketMaker::GetOutputAmount(uint64_t input_amount, uint64_t input_reserve,
                                               uint64_t output_reserve, uint64_t fee_rate) {
    
    // Validate fee_rate is reasonable (< 10000 = 100%)
    if (fee_rate >= 10000) {
        return 0;
    }
    
    // Check for zero reserves
    if (input_reserve == 0 || output_reserve == 0) {
        return 0;
    }
    
    // Apply fee with SafeMath
    uint64_t fee_multiplier = 10000 - fee_rate;
    auto input_with_fee_opt = SafeMath::Mul(input_amount, fee_multiplier);
    if (!input_with_fee_opt) {
        return 0;  // Overflow
    }
    uint64_t input_with_fee = *input_with_fee_opt;
    
    // Calculate numerator with SafeMath
    auto numerator_opt = SafeMath::Mul(input_with_fee, output_reserve);
    if (!numerator_opt) {
        return 0;  // Overflow
    }
    uint64_t numerator = *numerator_opt;
    
    // Calculate denominator with SafeMath
    auto reserve_scaled_opt = SafeMath::Mul(input_reserve, 10000);
    if (!reserve_scaled_opt) {
        return 0;  // Overflow
    }
    
    auto denominator_opt = SafeMath::Add(*reserve_scaled_opt, input_with_fee);
    if (!denominator_opt) {
        return 0;  // Overflow
    }
    uint64_t denominator = *denominator_opt;
    
    // Safe division
    auto result_opt = SafeMath::Div(numerator, denominator);
    if (!result_opt) {
        return 0;  // Division by zero (shouldn't happen due to earlier checks)
    }
    
    return *result_opt;
}

double AutomatedMarketMaker::GetPrice(const std::vector<uint8_t>& pool_id,
                                      primitives::AssetID asset) {
    auto pool_opt = GetPool(pool_id);
    if (!pool_opt) {
        return 0.0;
    }

    auto& pool = *pool_opt;

    
    // Check for division by zero
    if (asset == pool.asset_a) {
        if (pool.reserve_a == 0) {
            return 0.0;
        }
        return static_cast<double>(pool.reserve_b) / static_cast<double>(pool.reserve_a);
    } else {
        if (pool.reserve_b == 0) {
            return 0.0;
        }
        return static_cast<double>(pool.reserve_a) / static_cast<double>(pool.reserve_b);
    }
}

// DEXManager implementation
DEXManager::DEXManager() {}
DEXManager::~DEXManager() {}

OrderBook* DEXManager::GetOrderBook(primitives::AssetID base, primitives::AssetID quote) {
    auto key = std::make_pair(base, quote);
    auto it = order_books_.find(key);

    if (it == order_books_.end()) {
        order_books_.emplace(key, OrderBook(base, quote));
        it = order_books_.find(key);
    }

    return &it->second;
}

std::vector<Trade> DEXManager::ExecuteMarketOrder(primitives::AssetID base,
                                                  primitives::AssetID quote, OrderType type,
                                                  uint64_t amount,
                                                  const std::vector<uint8_t>& trader_pubkey) {
    auto* book = GetOrderBook(base, quote);

    Order order;
    order.trader_pubkey = trader_pubkey;
    order.base_asset = base;
    order.quote_asset = quote;
    order.type = type;
    order.amount = amount;
    order.price = (type == OrderType::MARKET_BUY) ? UINT64_MAX : 0;

    book->PlaceOrder(order);
    return book->MatchOrders();
}

std::vector<std::pair<primitives::AssetID, primitives::AssetID>>
DEXManager::GetTradingPairs() const {
    std::vector<std::pair<primitives::AssetID, primitives::AssetID>> pairs;
    for (const auto& entry : order_books_) {
        pairs.push_back(entry.first);
    }
    return pairs;
}

std::vector<Trade> DEXManager::GetRecentTrades(primitives::AssetID base, primitives::AssetID quote,
                                               size_t count) const {
    std::vector<Trade> recent;
    for (auto it = trade_history_.rbegin(); it != trade_history_.rend() && recent.size() < count;
         ++it) {
        if (it->base_asset == base && it->quote_asset == quote) {
            recent.push_back(*it);
        }
    }
    return recent;
}

uint64_t DEXManager::Get24HVolume(primitives::AssetID base, primitives::AssetID quote) const {
    uint64_t now = static_cast<uint64_t>(std::time(nullptr));
    
    // Check for underflow
    uint64_t day_ago = (now >= 86400) ? (now - 86400) : 0;
    
    uint64_t volume = 0;
    for (const auto& trade : trade_history_) {
        if (trade.base_asset == base && trade.quote_asset == quote &&
            trade.timestamp >= day_ago) {
            // Check for overflow before adding
            if (volume > UINT64_MAX - trade.amount) {
                volume = UINT64_MAX;  // Cap at maximum
                break;
            }
            volume += trade.amount;
        }
    }
    return volume;
}

}  // namespace dex
}  // namespace layer2
}  // namespace parthenon
