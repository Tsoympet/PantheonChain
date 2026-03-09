#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace pantheon::common {

// ------------------------------------------------------------------ //
//  MetricsRegistry — thread-safe Prometheus-compatible metrics store  //
// ------------------------------------------------------------------ //

/**
 * A single-label (label=value) tag attached to a metric sample.
 * Example: {layer="l1"}, {method="getinfo"}
 */
struct MetricLabel {
    std::string name;
    std::string value;
};

class MetricsRegistry {
  public:
    // ---- Counters (monotonically increasing) -------------------------

    /** Increment a counter by `amount`. */
    void Increment(const std::string& key, uint64_t amount = 1);
    void Increment(const std::string& key,
                   const std::vector<MetricLabel>& labels,
                   uint64_t amount = 1);

    /** Read current counter value. */
    uint64_t Read(const std::string& key) const;

    // ---- Gauges (arbitrary value, goes up and down) ------------------

    /** Set a gauge to an explicit value. */
    void SetGauge(const std::string& key, double value);
    void SetGauge(const std::string& key,
                  const std::vector<MetricLabel>& labels,
                  double value);

    /** Read current gauge value (returns 0.0 if not set). */
    double ReadGauge(const std::string& key) const;

    // ---- Histograms (observe a measurement, auto-bucket) -------------

    /**
     * Record an observation into a histogram.
     * Default buckets: {0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1, 5, 10, +Inf} seconds.
     */
    void Observe(const std::string& key, double value);
    void Observe(const std::string& key,
                 const std::vector<MetricLabel>& labels,
                 double value);

    // ---- Prometheus text-format export --------------------------------

    /**
     * Render all metrics in the Prometheus text exposition format (v0.0.4).
     * Expose this string on a GET /metrics HTTP endpoint.
     */
    std::string PrometheusText() const;

  private:
    mutable std::mutex mu_;

    // Counters
    std::unordered_map<std::string, uint64_t> counters_;

    // Gauges
    std::unordered_map<std::string, double> gauges_;

    // Histograms: key → {sum, count, bucket_counts[]}
    struct HistogramData {
        double sum{0.0};
        uint64_t count{0};
        // Bucket upper bounds: last element is +Inf sentinel (1e308)
        std::vector<double> bounds{0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1.0, 5.0, 10.0, 1e308};
        std::vector<uint64_t> bucket_counts;
        HistogramData() : bucket_counts(10, 0) {}
    };
    std::unordered_map<std::string, HistogramData> histograms_;

    static std::string EscapeLabel(const std::string& s);
    static std::string LabelsToString(const std::vector<MetricLabel>& labels);
    static std::string BuildKey(const std::string& base,
                                const std::vector<MetricLabel>& labels);
};

// ------------------------------------------------------------------ //
//  Convenience RAII timer — records elapsed time into a histogram     //
// ------------------------------------------------------------------ //
class ScopedTimer {
  public:
    ScopedTimer(MetricsRegistry& reg, std::string key)
        : reg_(reg), key_(std::move(key)),
          start_(std::chrono::steady_clock::now()) {}

    ~ScopedTimer() {
        using namespace std::chrono;
        double elapsed_s =
            duration<double>(steady_clock::now() - start_).count();
        reg_.Observe(key_, elapsed_s);
    }

  private:
    MetricsRegistry& reg_;
    std::string key_;
    std::chrono::steady_clock::time_point start_;
};

}  // namespace pantheon::common
