#ifndef PARTHENON_TOOLS_TESTING_FRAMEWORK_H
#define PARTHENON_TOOLS_TESTING_FRAMEWORK_H

#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <map>

namespace parthenon {
namespace testing {

struct TestResult {
    std::string test_name;
    bool passed;
    std::string error_message;
    uint64_t execution_time_us;
    
    TestResult() : passed(false), execution_time_us(0) {}
};

class TestSuite {
public:
    using TestFunction = std::function<void()>;
    
    TestSuite(const std::string& name);
    ~TestSuite();
    
    void AddTest(const std::string& name, TestFunction test);
    std::vector<TestResult> RunAll();
    TestResult RunTest(const std::string& name);
    size_t GetTestCount() const { return tests_.size(); }
    
private:
    std::string suite_name_;
    std::map<std::string, TestFunction> tests_;
    TestFunction setup_;
    TestFunction teardown_;
};

} // namespace testing
} // namespace parthenon

#endif // PARTHENON_TOOLS_TESTING_FRAMEWORK_H
