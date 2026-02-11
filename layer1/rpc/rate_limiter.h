// ParthenonChain - RPC Rate Limiter
// Prevents abuse by limiting requests per IP address

#ifndef PARTHENON_RPC_RATE_LIMITER_H
#define PARTHENON_RPC_RATE_LIMITER_H

#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>

namespace parthenon {
namespace rpc {

/**
 * Request tracking information for rate limiting
 */
struct RateLimitInfo {
    uint32_t request_count;
    std::chrono::steady_clock::time_point window_start;
    std::chrono::steady_clock::time_point last_request;
    
    RateLimitInfo() 
        : request_count(0),
          window_start(std::chrono::steady_clock::now()),
          last_request(std::chrono::steady_clock::now()) {}
};

/**
 * Rate limiter for RPC endpoints
 * Implements token bucket algorithm with per-IP tracking
 */
class RateLimiter {
  public:
    /**
     * Constructor
     * @param requests_per_window Maximum requests allowed per time window
     * @param window_seconds Time window in seconds
     * @param burst_size Maximum burst size (default = requests_per_window)
     */
    RateLimiter(uint32_t requests_per_window = 100, 
                uint32_t window_seconds = 60,
                uint32_t burst_size = 0)
        : max_requests_(requests_per_window),
          window_duration_(std::chrono::seconds(window_seconds)),
          burst_size_(burst_size > 0 ? burst_size : requests_per_window) {}

    /**
     * Check if a request from an IP should be allowed
     * @param ip_address Client IP address
     * @return true if request is allowed, false if rate limit exceeded
     */
    bool AllowRequest(const std::string& ip_address) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        auto& info = clients_[ip_address];
        
        // Check if we need to reset the window
        auto elapsed = now - info.window_start;
        if (elapsed >= window_duration_) {
            info.request_count = 0;
            info.window_start = now;
        }
        
        // Check rate limit
        if (info.request_count >= max_requests_) {
            return false;  // Rate limit exceeded
        }
        
        // Check burst limit (prevent too many requests in short time)
        auto time_since_last = now - info.last_request;
        if (time_since_last < std::chrono::milliseconds(100) && 
            info.request_count >= burst_size_) {
            return false;  // Burst limit exceeded
        }
        
        // Allow request
        info.request_count++;
        info.last_request = now;
        
        return true;
    }

    /**
     * Get current request count for an IP
     * @param ip_address Client IP address
     * @return Number of requests in current window
     */
    uint32_t GetRequestCount(const std::string& ip_address) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = clients_.find(ip_address);
        if (it == clients_.end()) {
            return 0;
        }
        return it->second.request_count;
    }

    /**
     * Reset rate limit for an IP (admin use)
     * @param ip_address Client IP address
     */
    void ResetIP(const std::string& ip_address) {
        std::lock_guard<std::mutex> lock(mutex_);
        clients_.erase(ip_address);
    }

    /**
     * Clean up old entries (call periodically)
     * Removes entries that haven't been used recently
     */
    void Cleanup() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        auto cleanup_threshold = std::chrono::hours(1);
        
        for (auto it = clients_.begin(); it != clients_.end();) {
            if (now - it->second.last_request > cleanup_threshold) {
                it = clients_.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * Get number of tracked IPs
     */
    size_t GetTrackedIPCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return clients_.size();
    }

  private:
    uint32_t max_requests_;
    std::chrono::seconds window_duration_;
    uint32_t burst_size_;
    
    mutable std::mutex mutex_;
    std::map<std::string, RateLimitInfo> clients_;
};

}  // namespace rpc
}  // namespace parthenon

#endif  // PARTHENON_RPC_RATE_LIMITER_H
