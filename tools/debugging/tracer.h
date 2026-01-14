#ifndef PARTHENON_TOOLS_DEBUGGING_TRACER_H
#define PARTHENON_TOOLS_DEBUGGING_TRACER_H

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <optional>

namespace parthenon {
namespace tools {
namespace debugging {

/**
 * Transaction Tracer
 * Traces transaction execution for debugging
 */
class TransactionTracer {
public:
    struct TraceStep {
        uint64_t step_number;
        uint64_t gas_used;
        uint64_t gas_remaining;
        uint8_t opcode;
        std::string opcode_name;
        std::vector<uint8_t> stack;
        std::vector<uint8_t> memory;
        std::map<uint32_t, std::vector<uint8_t>> storage;
        uint64_t program_counter;
    };
    
    struct TraceResult {
        std::vector<TraceStep> steps;
        bool success;
        std::string error_message;
        uint64_t total_gas_used;
        std::vector<uint8_t> return_data;
    };
    
    /**
     * Trace transaction execution
     */
    static TraceResult TraceTransaction(
        const std::vector<uint8_t>& bytecode,
        const std::vector<uint8_t>& input_data,
        uint64_t gas_limit
    );
    
    /**
     * Trace contract call
     */
    static TraceResult TraceCall(
        const std::vector<uint8_t>& contract_address,
        const std::vector<uint8_t>& call_data,
        uint64_t gas_limit
    );
    
    /**
     * Get opcode name
     */
    static std::string GetOpcodeName(uint8_t opcode);
};

/**
 * State Debugger
 * Debug blockchain state
 */
class StateDebugger {
public:
    /**
     * Get state at block height
     */
    struct StateSnapshot {
        uint64_t block_height;
        std::map<std::vector<uint8_t>, uint64_t> balances;
        std::map<std::vector<uint8_t>, std::vector<uint8_t>> storage;
        std::map<std::vector<uint8_t>, std::vector<uint8_t>> code;
    };
    
    static StateSnapshot GetStateAt(uint64_t block_height);
    
    /**
     * Compare states at two heights
     */
    struct StateDiff {
        std::map<std::vector<uint8_t>, int64_t> balance_changes;
        std::map<std::vector<uint8_t>, std::vector<uint8_t>> storage_changes;
    };
    
    static StateDiff CompareSates(uint64_t height1, uint64_t height2);
    
    /**
     * Get account state
     */
    struct AccountState {
        uint64_t balance;
        uint64_t nonce;
        std::vector<uint8_t> code;
        std::map<uint32_t, std::vector<uint8_t>> storage;
    };
    
    static std::optional<AccountState> GetAccount(
        const std::vector<uint8_t>& address,
        uint64_t block_height
    );
};

/**
 * Profiler
 * Profile transaction and block processing
 */
class Profiler {
public:
    struct ProfileResult {
        uint64_t total_time_us;
        uint64_t validation_time_us;
        uint64_t execution_time_us;
        uint64_t state_update_time_us;
        uint64_t gas_used;
        uint64_t opcodes_executed;
        std::map<std::string, uint64_t> opcode_counts;
        std::map<std::string, uint64_t> opcode_times;
    };
    
    /**
     * Profile transaction execution
     */
    static ProfileResult ProfileTransaction(
        const std::vector<uint8_t>& tx_data
    );
    
    /**
     * Profile block processing
     */
    static ProfileResult ProfileBlock(uint64_t block_height);
    
    /**
     * Get performance bottlenecks
     */
    static std::vector<std::string> GetBottlenecks(
        const ProfileResult& result
    );
};

/**
 * Event Logger
 * Log blockchain events for debugging
 */
class EventLogger {
public:
    enum class EventType {
        TRANSACTION,
        BLOCK,
        STATE_CHANGE,
        ERROR,
        WARNING
    };
    
    struct Event {
        EventType type;
        uint64_t timestamp;
        std::string message;
        std::map<std::string, std::string> metadata;
    };
    
    /**
     * Log event
     */
    static void LogEvent(const Event& event);
    
    /**
     * Get events by type
     */
    static std::vector<Event> GetEvents(
        EventType type,
        uint64_t start_time = 0,
        uint64_t end_time = UINT64_MAX
    );
    
    /**
     * Clear logs
     */
    static void ClearLogs();
    
private:
    static std::vector<Event> events_;
};

} // namespace debugging
} // namespace tools
} // namespace parthenon

#endif // PARTHENON_TOOLS_DEBUGGING_TRACER_H
