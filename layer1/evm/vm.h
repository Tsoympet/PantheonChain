// ParthenonChain - EVM Virtual Machine
// Full EVM implementation with deterministic execution

#pragma once

#include "opcodes.h"
#include "state.h"

#include <memory>
#include <optional>
#include <stack>
#include <vector>
#include <cstddef>

namespace parthenon {
namespace evm {

/**
 * Execution result
 */
enum class ExecResult {
    SUCCESS,   // Normal completion
    RETURNED,  // Explicit RETURN
    REVERT,
    OUT_OF_GAS,
    STACK_UNDERFLOW,
    STACK_OVERFLOW,
    INVALID_JUMP,
    INVALID_OPCODE,
    STATIC_CALL_VIOLATION,
    DEPTH_EXCEEDED,
};

/**
 * Execution context for VM
 */
struct ExecutionContext {
    Address origin;                   // Transaction sender
    Address caller;                   // Immediate caller
    Address address;                  // Current contract address
    uint256_t value;                  // OBL value sent
    std::vector<uint8_t> input_data;  // Call data
    uint64_t gas_limit;               // Gas available
    uint64_t gas_price;               // Gas price (OBL per gas)
    uint64_t block_number;            // Current block number
    uint64_t timestamp;               // Block timestamp
    Address coinbase;                 // Block miner
    uint64_t difficulty;              // Block difficulty
    uint64_t gas_limit_block;         // Block gas limit
    uint64_t chain_id;                // Chain ID
    uint64_t base_fee;                // EIP-1559 base fee
    bool is_static;                   // Static call flag
    uint32_t depth;                   // Call depth
};

/**
 * Virtual Machine - executes EVM bytecode
 */
class VM {
  public:
    VM(WorldState& state, const ExecutionContext& ctx);

    /**
     * Execute contract code
     *
     * @param code Contract bytecode
     * @return Execution result and return data
     */
    std::pair<ExecResult, std::vector<uint8_t>> Execute(const std::vector<uint8_t>& code);

    /**
     * Get gas used
     */
    uint64_t GetGasUsed() const { return gas_used_; }

    /**
     * Get logs generated
     */
    const std::vector<LogEntry>& GetLogs() const { return logs_; }

  private:
    // Stack operations
    void Push(const uint256_t& value);
    uint256_t Pop();
    uint256_t Peek(size_t depth = 0) const;
    void Dup(uint8_t depth);
    void Swap(uint8_t depth);

    // Memory operations
    void MemoryStore(uint64_t offset, const uint256_t& value);
    void MemoryStore8(uint64_t offset, uint8_t value);
    uint256_t MemoryLoad(uint64_t offset);
    void ExpandMemory(uint64_t size);
    uint64_t GetMemorySize() const { return memory_.size(); }

    // Gas operations
    bool UseGas(uint64_t amount);
    uint64_t GetGasRemaining() const;

    // Arithmetic operations
    uint256_t Add(const uint256_t& a, const uint256_t& b) const;
    uint256_t Sub(const uint256_t& a, const uint256_t& b) const;
    uint256_t Mul(const uint256_t& a, const uint256_t& b) const;
    uint256_t Div(const uint256_t& a, const uint256_t& b) const;
    uint256_t Mod(const uint256_t& a, const uint256_t& b) const;
    uint256_t Exp(const uint256_t& base, const uint256_t& exponent) const;

    // Comparison operations
    bool Lt(const uint256_t& a, const uint256_t& b) const;
    bool Gt(const uint256_t& a, const uint256_t& b) const;
    bool Eq(const uint256_t& a, const uint256_t& b) const;
    bool IsZero(const uint256_t& a) const;

    // Bitwise operations
    uint256_t And(const uint256_t& a, const uint256_t& b) const;
    uint256_t Or(const uint256_t& a, const uint256_t& b) const;
    uint256_t Xor(const uint256_t& a, const uint256_t& b) const;
    uint256_t Not(const uint256_t& a) const;
    uint256_t Shl(const uint256_t& shift, const uint256_t& value) const;
    uint256_t Shr(const uint256_t& shift, const uint256_t& value) const;

    // Execute single opcode
    ExecResult ExecuteOpcode(Opcode op, const std::vector<uint8_t>& code, size_t& pc);

    // Call operations
    std::pair<ExecResult, std::vector<uint8_t>> Call(const Address& target, const uint256_t& value,
                                                     const std::vector<uint8_t>& input,
                                                     uint64_t gas, bool is_static);

    WorldState& state_;
    ExecutionContext ctx_;

    std::vector<uint256_t> stack_;
    std::vector<uint8_t> memory_;
    std::vector<uint8_t> return_data_;
    std::vector<LogEntry> logs_;

    uint64_t gas_used_;

    static constexpr size_t MAX_STACK_SIZE = 1024;
    static constexpr size_t MAX_CALL_DEPTH = 1024;
};

}  // namespace evm
}  // namespace parthenon
