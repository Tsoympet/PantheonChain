// ParthenonChain - Chaos Engineering Test Suite
// Systematic resilience testing with fault injection

#pragma once

#include <functional>
#include <random>
#include <string>
#include <vector>

namespace parthenon {
namespace testing {

/**
 * Chaos engineering test results
 */
struct ChaosTestResult {
    std::string test_name;
    bool passed;
    std::string error_message;
    double duration_seconds;
    size_t iterations;
};

/**
 * Chaos Engineering Test Framework
 * Injects faults to test system resilience
 */
class ChaosEngineering {
  public:
    /**
     * Initialize chaos testing framework
     */
    bool Init();

    /**
     * Network failure injection tests
     */
    ChaosTestResult TestNetworkPartition();
    ChaosTestResult TestPacketLoss(double loss_rate = 0.1);
    ChaosTestResult TestNetworkLatency(uint32_t latency_ms = 1000);
    ChaosTestResult TestBandwidthLimit(uint64_t bytes_per_sec = 100000);

    /**
     * Storage failure tests
     */
    ChaosTestResult TestDiskFull();
    ChaosTestResult TestCorruptedDatabase();
    ChaosTestResult TestSlowIO(uint32_t delay_ms = 100);

    /**
     * Peer behavior tests
     */
    ChaosTestResult TestMaliciousPeer();
    ChaosTestResult TestSlowPeer(uint32_t delay_ms = 5000);
    ChaosTestResult TestDisconnectingPeers();

    /**
     * Consensus tests
     */
    ChaosTestResult TestForkResolution();
    ChaosTestResult TestOrphanBlocks();
    ChaosTestResult TestDoubleSpend();

    /**
     * Resource exhaustion tests
     */
    ChaosTestResult TestMemoryPressure(size_t bytes = 1024 * 1024 * 1024);
    ChaosTestResult TestCPUStarvation();
    ChaosTestResult TestFileDescriptorExhaustion();

    /**
     * Timing and race condition tests
     */
    ChaosTestResult TestRaceConditions();
    ChaosTestResult TestDeadlocks();

    /**
     * Run all chaos tests
     */
    std::vector<ChaosTestResult> RunAllTests();

    /**
     * Generate chaos testing report
     */
    std::string GenerateReport(const std::vector<ChaosTestResult> &results);

  private:
    std::mt19937 rng_;
    bool initialized_ = false;

    // Helper functions
    void InjectNetworkFault(const std::string &fault_type);
    void RemoveNetworkFault();
    bool VerifySystemRecovery();
};

} // namespace testing
} // namespace parthenon
