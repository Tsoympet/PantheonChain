// ParthenonChain - Decentralized Exchange (DEX)
// On-chain orderbook and automated market maker

#pragma once

#include "primitives/asset.h"
#include "primitives/transaction.h"

#include <cstdint>
#include <map>
#include <optional>
#include <utility>
#include <vector>
#include <cstddef>

namespace parthenon {
namespace layer2 {
namespace dex {

/**
 * Order Types
 */
enum class OrderType {
    LIMIT_BUY,   // Buy at or below price
    LIMIT_SELL,  // Sell at or above price
    MARKET_BUY,  // Buy at best available price
    MARKET_SELL  // Sell at best available price
};

enum class OrderStatus {
    PENDING,   // Order placed but not matched
    PARTIAL,   // Partially filled
    FILLED,    // Completely filled
    CANCELLED  // Cancelled by user
};

/**
 * Order Book Entry
 */
struct Order {
    std::vector<uint8_t> order_id;
    std::vector<uint8_t> trader_pubkey;
    primitives::AssetID base_asset;   // Asset being traded
    primitives::AssetID quote_asset;  // Asset used for pricing
    OrderType type;
    OrderStatus status;
    uint64_t price;          // Price in quote asset units
    uint64_t amount;         // Amount of base asset
    uint64_t filled_amount;  // Amount already filled
    uint64_t timestamp;
    std::vector<uint8_t> signature;  // Schnorr signature

    Order() : price(0), amount(0), filled_amount(0), timestamp(0) {}
};

/**
 * Trade execution result
 */
struct Trade {
    std::vector<uint8_t> trade_id;
    std::vector<uint8_t> buy_order_id;
    std::vector<uint8_t> sell_order_id;
    primitives::AssetID base_asset;
    primitives::AssetID quote_asset;
    uint64_t price;
    uint64_t amount;
    uint64_t timestamp;
    std::vector<uint8_t> buyer_pubkey;
    std::vector<uint8_t> seller_pubkey;
};

/**
 * Liquidity Pool for AMM
 */
struct LiquidityPool {
    std::vector<uint8_t> pool_id;
    primitives::AssetID asset_a;
    primitives::AssetID asset_b;
    uint64_t reserve_a;                               // Reserve of asset A
    uint64_t reserve_b;                               // Reserve of asset B
    uint64_t total_shares;                            // Total LP token shares
    std::map<std::vector<uint8_t>, uint64_t> shares;  // Provider shares
    uint64_t fee_rate;                                // Fee in basis points (e.g., 30 = 0.3%)

    LiquidityPool() : reserve_a(0), reserve_b(0), total_shares(0), fee_rate(30) {}
};

/**
 * Order Book
 * Manages limit orders for a trading pair
 */
class OrderBook {
  public:
    OrderBook(primitives::AssetID base, primitives::AssetID quote);

    /**
     * Place limit order
     */
    std::vector<uint8_t> PlaceOrder(const Order& order);

    /**
     * Cancel order
     */
    bool CancelOrder(const std::vector<uint8_t>& order_id,
                     const std::vector<uint8_t>& trader_pubkey);

    /**
     * Match orders and execute trades
     */
    std::vector<Trade> MatchOrders();

    /**
     * Get best bid (highest buy price)
     */
    std::optional<Order> GetBestBid() const;

    /**
     * Get best ask (lowest sell price)
     */
    std::optional<Order> GetBestAsk() const;

    /**
     * Get order book depth
     */
    struct MarketDepth {
        std::vector<std::pair<uint64_t, uint64_t>> bids;  // (price, total_amount)
        std::vector<std::pair<uint64_t, uint64_t>> asks;
    };

    MarketDepth GetDepth(size_t levels = 10) const;

    /**
     * Get order by ID
     */
    std::optional<Order> GetOrder(const std::vector<uint8_t>& order_id) const;

  private:
    primitives::AssetID base_asset_;
    primitives::AssetID quote_asset_;
    std::map<uint64_t, std::vector<Order>> buy_orders_;   // Price -> Orders
    std::map<uint64_t, std::vector<Order>> sell_orders_;  // Price -> Orders
    std::map<std::vector<uint8_t>, Order> orders_by_id_;

    bool ValidateOrder(const Order& order) const;
    void RemoveOrder(const std::vector<uint8_t>& order_id);
};

/**
 * Automated Market Maker (AMM)
 * Constant product (x * y = k) formula
 */
class AutomatedMarketMaker {
  public:
    /**
     * Create liquidity pool
     */
    static std::vector<uint8_t> CreatePool(primitives::AssetID asset_a, primitives::AssetID asset_b,
                                           uint64_t initial_a, uint64_t initial_b,
                                           uint64_t fee_rate = 30);

    /**
     * Add liquidity to pool
     */
    static uint64_t AddLiquidity(const std::vector<uint8_t>& pool_id, uint64_t amount_a,
                                 uint64_t amount_b, const std::vector<uint8_t>& provider_pubkey);

    /**
     * Remove liquidity from pool
     */
    static std::pair<uint64_t, uint64_t>
    RemoveLiquidity(const std::vector<uint8_t>& pool_id, uint64_t shares,
                    const std::vector<uint8_t>& provider_pubkey);

    /**
     * Swap tokens using AMM
     */
    static uint64_t Swap(const std::vector<uint8_t>& pool_id, primitives::AssetID input_asset,
                         uint64_t input_amount, uint64_t min_output_amount);

    /**
     * Get pool by ID
     */
    static std::optional<LiquidityPool> GetPool(const std::vector<uint8_t>& pool_id);

    /**
     * Calculate output amount for swap
     */
    static uint64_t GetOutputAmount(uint64_t input_amount, uint64_t input_reserve,
                                    uint64_t output_reserve, uint64_t fee_rate);

    /**
     * Get current price in pool
     */
    static double GetPrice(const std::vector<uint8_t>& pool_id, primitives::AssetID asset);

  private:
    static std::map<std::vector<uint8_t>, LiquidityPool> pools_;
};

/**
 * DEX Manager
 * High-level interface for decentralized exchange
 */
class DEXManager {
  public:
    DEXManager();
    ~DEXManager();

    /**
     * Get or create order book for trading pair
     */
    OrderBook* GetOrderBook(primitives::AssetID base, primitives::AssetID quote);

    /**
     * Execute market order
     */
    std::vector<Trade> ExecuteMarketOrder(primitives::AssetID base, primitives::AssetID quote,
                                          OrderType type, uint64_t amount,
                                          const std::vector<uint8_t>& trader_pubkey);

    /**
     * Get all trading pairs
     */
    std::vector<std::pair<primitives::AssetID, primitives::AssetID>> GetTradingPairs() const;

    /**
     * Get recent trades
     */
    std::vector<Trade> GetRecentTrades(primitives::AssetID base, primitives::AssetID quote,
                                       size_t count = 50) const;

    /**
     * Get 24h volume
     */
    uint64_t Get24HVolume(primitives::AssetID base, primitives::AssetID quote) const;

  private:
    std::map<std::pair<primitives::AssetID, primitives::AssetID>, OrderBook> order_books_;
    std::vector<Trade> trade_history_;
};

}  // namespace dex
}  // namespace layer2
}  // namespace parthenon
