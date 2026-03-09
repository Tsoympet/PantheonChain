#include "metrics.h"

#include <algorithm>
#include <sstream>

namespace pantheon::common {

// ------------------------------------------------------------------ //
//  Helpers                                                             //
// ------------------------------------------------------------------ //

std::string MetricsRegistry::EscapeLabel(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"')  { out += "\\\""; }
        else if (c == '\n') { out += "\\n"; }
        else if (c == '\\') { out += "\\\\"; }
        else { out += c; }
    }
    return out;
}

std::string MetricsRegistry::LabelsToString(const std::vector<MetricLabel>& labels) {
    if (labels.empty()) return "";
    std::ostringstream oss;
    oss << '{';
    for (size_t i = 0; i < labels.size(); ++i) {
        if (i > 0) oss << ',';
        oss << labels[i].name << "=\"" << EscapeLabel(labels[i].value) << '"';
    }
    oss << '}';
    return oss.str();
}

std::string MetricsRegistry::BuildKey(const std::string& base,
                                      const std::vector<MetricLabel>& labels) {
    if (labels.empty()) return base;
    return base + LabelsToString(labels);
}

// ------------------------------------------------------------------ //
//  Counters                                                            //
// ------------------------------------------------------------------ //

void MetricsRegistry::Increment(const std::string& key, uint64_t amount) {
    std::lock_guard<std::mutex> lk(mu_);
    counters_[key] += amount;
}

void MetricsRegistry::Increment(const std::string& key,
                                 const std::vector<MetricLabel>& labels,
                                 uint64_t amount) {
    Increment(BuildKey(key, labels), amount);
}

uint64_t MetricsRegistry::Read(const std::string& key) const {
    std::lock_guard<std::mutex> lk(mu_);
    const auto it = counters_.find(key);
    return it == counters_.end() ? 0 : it->second;
}

// ------------------------------------------------------------------ //
//  Gauges                                                              //
// ------------------------------------------------------------------ //

void MetricsRegistry::SetGauge(const std::string& key, double value) {
    std::lock_guard<std::mutex> lk(mu_);
    gauges_[key] = value;
}

void MetricsRegistry::SetGauge(const std::string& key,
                                const std::vector<MetricLabel>& labels,
                                double value) {
    SetGauge(BuildKey(key, labels), value);
}

double MetricsRegistry::ReadGauge(const std::string& key) const {
    std::lock_guard<std::mutex> lk(mu_);
    const auto it = gauges_.find(key);
    return it == gauges_.end() ? 0.0 : it->second;
}

// ------------------------------------------------------------------ //
//  Histograms                                                          //
// ------------------------------------------------------------------ //

void MetricsRegistry::Observe(const std::string& key, double value) {
    std::lock_guard<std::mutex> lk(mu_);
    auto& h = histograms_[key];
    h.sum += value;
    ++h.count;
    for (size_t i = 0; i < h.bounds.size(); ++i) {
        if (value <= h.bounds[i]) {
            ++h.bucket_counts[i];
        }
    }
}

void MetricsRegistry::Observe(const std::string& key,
                               const std::vector<MetricLabel>& labels,
                               double value) {
    Observe(BuildKey(key, labels), value);
}

// ------------------------------------------------------------------ //
//  Prometheus text export                                              //
// ------------------------------------------------------------------ //

std::string MetricsRegistry::PrometheusText() const {
    std::lock_guard<std::mutex> lk(mu_);
    std::ostringstream oss;

    // Counters
    for (const auto& [k, v] : counters_) {
        oss << "# TYPE " << k << " counter\n";
        oss << k << ' ' << v << '\n';
    }

    // Gauges
    for (const auto& [k, v] : gauges_) {
        oss << "# TYPE " << k << " gauge\n";
        oss << k << ' ' << v << '\n';
    }

    // Histograms
    for (const auto& [k, h] : histograms_) {
        oss << "# TYPE " << k << " histogram\n";
        for (size_t i = 0; i < h.bounds.size(); ++i) {
            oss << k << "_bucket{le=\"";
            if (h.bounds[i] >= 1e307) {
                oss << "+Inf";
            } else {
                oss << h.bounds[i];
            }
            oss << "\"} " << h.bucket_counts[i] << '\n';
        }
        oss << k << "_sum "   << h.sum   << '\n';
        oss << k << "_count " << h.count << '\n';
    }

    return oss.str();
}

}  // namespace pantheon::common
