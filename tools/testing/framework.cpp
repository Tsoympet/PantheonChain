#include "framework.h"
#include <chrono>
#include <stdexcept>

namespace parthenon {
namespace testing {

TestSuite::TestSuite(const std::string& name) : suite_name_(name) {}
TestSuite::~TestSuite() = default;

void TestSuite::AddTest(const std::string& name, TestFunction test) {
    tests_[name] = test;
}

std::vector<TestResult> TestSuite::RunAll() {
    std::vector<TestResult> results;
    for (const auto& [name, test] : tests_) {
        results.push_back(RunTest(name));
    }
    return results;
}

TestResult TestSuite::RunTest(const std::string& name) {
    TestResult result;
    result.test_name = name;
    
    auto it = tests_.find(name);
    if (it == tests_.end()) {
        result.passed = false;
        result.error_message = "Test not found";
        return result;
    }
    
    try {
        if (setup_) setup_();
        auto start = std::chrono::high_resolution_clock::now();
        it->second();
        auto end = std::chrono::high_resolution_clock::now();
        result.execution_time_us = 
            std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        if (teardown_) teardown_();
        result.passed = true;
    } catch (const std::exception& e) {
        result.passed = false;
        result.error_message = e.what();
    }
    
    return result;
}

} // namespace testing
} // namespace parthenon
