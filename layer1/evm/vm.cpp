// ParthenonChain - EVM Virtual Machine Implementation

#include "vm.h"
#include "crypto/sha256.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace parthenon {
namespace evm {

VM::VM(WorldState& state, const ExecutionContext& ctx)
    : state_(state), ctx_(ctx), gas_used_(0) {
    stack_.reserve(MAX_STACK_SIZE);
}

void VM::Push(const uint256_t& value) {
    if (stack_.size() >= MAX_STACK_SIZE) {
        throw std::runtime_error("Stack overflow");
    }
    stack_.push_back(value);
}

uint256_t VM::Pop() {
    if (stack_.empty()) {
        throw std::runtime_error("Stack underflow");
    }
    uint256_t value = stack_.back();
    stack_.pop_back();
    return value;
}

uint256_t VM::Peek(size_t depth) const {
    if (depth >= stack_.size()) {
        throw std::runtime_error("Stack underflow");
    }
    return stack_[stack_.size() - 1 - depth];
}

void VM::Dup(uint8_t depth) {
    if (depth == 0 || depth > stack_.size()) {
        throw std::runtime_error("Invalid DUP depth");
    }
    Push(Peek(depth - 1));
}

void VM::Swap(uint8_t depth) {
    if (depth == 0 || depth >= stack_.size()) {
        throw std::runtime_error("Invalid SWAP depth");
    }
    size_t top_idx = stack_.size() - 1;
    size_t swap_idx = stack_.size() - 1 - depth;
    std::swap(stack_[top_idx], stack_[swap_idx]);
}

void VM::ExpandMemory(uint64_t size) {
    if (size > memory_.size()) {
        // Calculate gas cost for memory expansion
        uint64_t old_words = (memory_.size() + 31) / 32;
        uint64_t new_words = (size + 31) / 32;
        uint64_t expansion_cost = (new_words - old_words) * 3;
        
        if (!UseGas(expansion_cost)) {
            throw std::runtime_error("Out of gas during memory expansion");
        }
        
        memory_.resize(size, 0);
    }
}

void VM::MemoryStore(uint64_t offset, const uint256_t& value) {
    ExpandMemory(offset + 32);
    std::memcpy(&memory_[offset], value.data(), 32);
}

void VM::MemoryStore8(uint64_t offset, uint8_t value) {
    ExpandMemory(offset + 1);
    memory_[offset] = value;
}

uint256_t VM::MemoryLoad(uint64_t offset) {
    ExpandMemory(offset + 32);
    uint256_t result;
    std::memcpy(result.data(), &memory_[offset], 32);
    return result;
}

bool VM::UseGas(uint64_t amount) {
    if (gas_used_ + amount > ctx_.gas_limit) {
        return false;
    }
    gas_used_ += amount;
    return true;
}

uint64_t VM::GetGasRemaining() const {
    return ctx_.gas_limit - gas_used_;
}

// Arithmetic operations - Full 256-bit implementation
// All operations use big-endian byte arrays for 256-bit integers
// Implements proper carry/borrow propagation for EVM compatibility

uint256_t VM::Add(const uint256_t& a, const uint256_t& b) const {
    uint256_t result{};
    uint16_t carry = 0;
    
    // Process bytes from least significant (index 31) to most significant (index 0)
    for (int i = 31; i >= 0; i--) {
        uint16_t sum = static_cast<uint16_t>(a[i]) + static_cast<uint16_t>(b[i]) + carry;
        result[i] = static_cast<uint8_t>(sum & 0xFF);
        carry = sum >> 8;
    }
    
    // Note: Overflow is allowed in EVM (wraps around modulo 2^256)
    return result;
}

uint256_t VM::Sub(const uint256_t& a, const uint256_t& b) const {
    uint256_t result{};
    int16_t borrow = 0;
    
    // Process bytes from least significant (index 31) to most significant (index 0)
    for (int i = 31; i >= 0; i--) {
        int16_t diff = static_cast<int16_t>(a[i]) - static_cast<int16_t>(b[i]) - borrow;
        if (diff < 0) {
            diff += 256;
            borrow = 1;
        } else {
            borrow = 0;
        }
        result[i] = static_cast<uint8_t>(diff);
    }
    
    // Note: Underflow wraps around modulo 2^256 (two's complement)
    return result;
}

uint256_t VM::Mul(const uint256_t& a, const uint256_t& b) const {
    // Use long multiplication algorithm for 256-bit values
    uint256_t result{};
    
    // For each byte in b (multiplier)
    for (int i = 31; i >= 0; i--) {
        if (b[i] == 0) continue;
        
        uint16_t carry = 0;
        // Multiply each byte of a by b[i]
        for (int j = 31; j >= 0; j--) {
            int result_idx = j + (31 - i);
            if (result_idx >= 32) continue; // Skip overflow
            
            uint32_t product = static_cast<uint32_t>(a[j]) * static_cast<uint32_t>(b[i]) + 
                               static_cast<uint32_t>(result[result_idx]) + carry;
            result[result_idx] = static_cast<uint8_t>(product & 0xFF);
            carry = product >> 8;
        }
    }
    
    return result;
}

uint256_t VM::Div(const uint256_t& a, const uint256_t& b) const {
    if (IsZero(b)) return uint256_t{}; // Division by zero returns 0 in EVM
    
    // Use long division algorithm for 256-bit values
    uint256_t quotient{};
    uint256_t remainder{};
    
    // Process bits from most significant to least significant
    for (int i = 0; i < 256; i++) {
        // Shift remainder left by 1
        uint16_t carry = 0;
        for (int j = 31; j >= 0; j--) {
            uint16_t shifted = (static_cast<uint16_t>(remainder[j]) << 1) | carry;
            remainder[j] = static_cast<uint8_t>(shifted & 0xFF);
            carry = shifted >> 8;
        }
        
        // Add current bit of dividend to remainder
        int byte_idx = i / 8;
        int bit_idx = 7 - (i % 8);
        if ((a[byte_idx] >> bit_idx) & 1) {
            remainder[31] |= 1;
        }
        
        // Check if remainder >= divisor
        bool ge = !Lt(remainder, b);
        if (ge) {
            // Subtract divisor from remainder
            remainder = Sub(remainder, b);
            
            // Set bit in quotient
            int quot_byte = i / 8;
            int quot_bit = 7 - (i % 8);
            quotient[quot_byte] |= (1 << quot_bit);
        }
    }
    
    return quotient;
}

uint256_t VM::Mod(const uint256_t& a, const uint256_t& b) const {
    if (IsZero(b)) return uint256_t{}; // Modulo by zero returns 0 in EVM
    
    // Use long division to get remainder
    uint256_t remainder{};
    
    // Process bits from most significant to least significant
    for (int i = 0; i < 256; i++) {
        // Shift remainder left by 1
        uint16_t carry = 0;
        for (int j = 31; j >= 0; j--) {
            uint16_t shifted = (static_cast<uint16_t>(remainder[j]) << 1) | carry;
            remainder[j] = static_cast<uint8_t>(shifted & 0xFF);
            carry = shifted >> 8;
        }
        
        // Add current bit of dividend to remainder
        int byte_idx = i / 8;
        int bit_idx = 7 - (i % 8);
        if ((a[byte_idx] >> bit_idx) & 1) {
            remainder[31] |= 1;
        }
        
        // Check if remainder >= divisor
        bool ge = !Lt(remainder, b);
        if (ge) {
            // Subtract divisor from remainder
            remainder = Sub(remainder, b);
        }
    }
    
    return remainder;
}

uint256_t VM::Exp(const uint256_t& base, const uint256_t& exponent) const {
    // Exponentiation by squaring (binary method)
    if (IsZero(exponent)) return ToUint256(1);
    if (IsZero(base)) return uint256_t{};
    
    uint256_t result = ToUint256(1);
    uint256_t current_base = base;
    uint256_t exp = exponent;
    
    // Process exponent bits from least to most significant
    for (int i = 0; i < 256; i++) {
        // Check if current bit of exponent is set
        int byte_idx = 31 - (i / 8);
        int bit_idx = i % 8;
        if ((exp[byte_idx] >> bit_idx) & 1) {
            result = Mul(result, current_base);
        }
        
        // Square the base for next iteration
        current_base = Mul(current_base, current_base);
        
        // Early exit if base becomes 0 or 1
        if (IsZero(current_base)) break;
        uint256_t one = ToUint256(1);
        if (Eq(current_base, one) && !Eq(result, one)) break;
    }
    
    return result;
}

bool VM::Lt(const uint256_t& a, const uint256_t& b) const {
    for (int i = 0; i < 32; i++) {
        if (a[i] < b[i]) return true;
        if (a[i] > b[i]) return false;
    }
    return false;
}

bool VM::Gt(const uint256_t& a, const uint256_t& b) const {
    return Lt(b, a);
}

bool VM::Eq(const uint256_t& a, const uint256_t& b) const {
    return std::memcmp(a.data(), b.data(), 32) == 0;
}

bool VM::IsZero(const uint256_t& a) const {
    for (uint8_t byte : a) {
        if (byte != 0) return false;
    }
    return true;
}

uint256_t VM::And(const uint256_t& a, const uint256_t& b) const {
    uint256_t result;
    for (size_t i = 0; i < 32; i++) {
        result[i] = a[i] & b[i];
    }
    return result;
}

uint256_t VM::Or(const uint256_t& a, const uint256_t& b) const {
    uint256_t result;
    for (size_t i = 0; i < 32; i++) {
        result[i] = a[i] | b[i];
    }
    return result;
}

uint256_t VM::Xor(const uint256_t& a, const uint256_t& b) const {
    uint256_t result;
    for (size_t i = 0; i < 32; i++) {
        result[i] = a[i] ^ b[i];
    }
    return result;
}

uint256_t VM::Not(const uint256_t& a) const {
    uint256_t result;
    for (size_t i = 0; i < 32; i++) {
        result[i] = ~a[i];
    }
    return result;
}

uint256_t VM::Shl(const uint256_t& shift, const uint256_t& value) const {
    uint64_t shift_amount = ToUint64(shift);
    if (shift_amount >= 256) return uint256_t{};
    
    // Simplified left shift
    uint256_t result = value;
    for (uint64_t i = 0; i < shift_amount; i++) {
        // Shift left by 1 bit
        uint8_t carry = 0;
        for (int j = 31; j >= 0; j--) {
            uint8_t new_carry = (result[j] & 0x80) >> 7;
            result[j] = (result[j] << 1) | carry;
            carry = new_carry;
        }
    }
    return result;
}

uint256_t VM::Shr(const uint256_t& shift, const uint256_t& value) const {
    uint64_t shift_amount = ToUint64(shift);
    if (shift_amount >= 256) return uint256_t{};
    
    // Simplified right shift
    uint256_t result = value;
    for (uint64_t i = 0; i < shift_amount; i++) {
        // Shift right by 1 bit
        uint8_t carry = 0;
        for (int j = 0; j < 32; j++) {
            uint8_t new_carry = (result[j] & 0x01) << 7;
            result[j] = (result[j] >> 1) | carry;
            carry = new_carry;
        }
    }
    return result;
}

std::pair<ExecResult, std::vector<uint8_t>> VM::Execute(const std::vector<uint8_t>& code) {
    size_t pc = 0; // Program counter
    std::vector<bool> jump_dests(code.size(), false);
    
    // Pre-scan for JUMPDEST
    for (size_t i = 0; i < code.size(); i++) {
        if (static_cast<Opcode>(code[i]) == Opcode::JUMPDEST) {
            jump_dests[i] = true;
        }
        // Skip PUSH data
        if (IsPushOp(static_cast<Opcode>(code[i]))) {
            i += GetPushSize(static_cast<Opcode>(code[i]));
        }
    }
    
    // Execute bytecode
    while (pc < code.size()) {
        Opcode op = static_cast<Opcode>(code[pc]);
        
        // Charge gas for opcode
        if (!UseGas(GetOpcodeCost(op))) {
            return {ExecResult::OUT_OF_GAS, {}};
        }
        
        try {
            ExecResult result = ExecuteOpcode(op, code, pc);
            
            if (result == ExecResult::SUCCESS) {
                // Continue execution
                pc++;
            } else {
                // Execution ended (RETURN, REVERT, etc.)
                return {result, return_data_};
            }
        } catch (const std::exception&) {
            return {ExecResult::INVALID_OPCODE, {}};
        }
    }
    
    // Reached end of code without explicit return
    return {ExecResult::SUCCESS, {}};
}

ExecResult VM::ExecuteOpcode(Opcode op, const std::vector<uint8_t>& code, size_t& pc) {
    switch (op) {
        case Opcode::STOP:
            return ExecResult::SUCCESS;
        
        case Opcode::ADD: {
            auto b = Pop();
            auto a = Pop();
            Push(Add(a, b));
            break;
        }
        
        case Opcode::MUL: {
            auto b = Pop();
            auto a = Pop();
            Push(Mul(a, b));
            break;
        }
        
        case Opcode::SUB: {
            auto b = Pop();
            auto a = Pop();
            Push(Sub(a, b));
            break;
        }
        
        case Opcode::DIV: {
            auto b = Pop();
            auto a = Pop();
            Push(Div(a, b));
            break;
        }
        
        case Opcode::MOD: {
            auto b = Pop();
            auto a = Pop();
            Push(Mod(a, b));
            break;
        }
        
        case Opcode::EXP: {
            auto exponent = Pop();
            auto base = Pop();
            Push(Exp(base, exponent));
            break;
        }
        
        case Opcode::LT: {
            auto b = Pop();
            auto a = Pop();
            Push(Lt(a, b) ? ToUint256(1) : ToUint256(0));
            break;
        }
        
        case Opcode::GT: {
            auto b = Pop();
            auto a = Pop();
            Push(Gt(a, b) ? ToUint256(1) : ToUint256(0));
            break;
        }
        
        case Opcode::EQ: {
            auto b = Pop();
            auto a = Pop();
            Push(Eq(a, b) ? ToUint256(1) : ToUint256(0));
            break;
        }
        
        case Opcode::ISZERO: {
            auto a = Pop();
            Push(IsZero(a) ? ToUint256(1) : ToUint256(0));
            break;
        }
        
        case Opcode::AND: {
            auto b = Pop();
            auto a = Pop();
            Push(And(a, b));
            break;
        }
        
        case Opcode::OR: {
            auto b = Pop();
            auto a = Pop();
            Push(Or(a, b));
            break;
        }
        
        case Opcode::XOR: {
            auto b = Pop();
            auto a = Pop();
            Push(Xor(a, b));
            break;
        }
        
        case Opcode::NOT: {
            auto a = Pop();
            Push(Not(a));
            break;
        }
        
        case Opcode::SHL: {
            auto value = Pop();
            auto shift = Pop();
            Push(Shl(shift, value));
            break;
        }
        
        case Opcode::SHR: {
            auto value = Pop();
            auto shift = Pop();
            Push(Shr(shift, value));
            break;
        }
        
        // Memory operations
        case Opcode::MLOAD: {
            auto offset = ToUint64(Pop());
            Push(MemoryLoad(offset));
            break;
        }
        
        case Opcode::MSTORE: {
            auto offset = ToUint64(Pop());
            auto value = Pop();
            MemoryStore(offset, value);
            break;
        }
        
        case Opcode::MSTORE8: {
            auto offset = ToUint64(Pop());
            auto value = Pop();
            MemoryStore8(offset, static_cast<uint8_t>(ToUint64(value) & 0xFF));
            break;
        }
        
        // Storage operations
        case Opcode::SLOAD: {
            auto key = Pop();
            auto value = state_.GetStorage(ctx_.address, key);
            Push(value);
            break;
        }
        
        case Opcode::SSTORE: {
            if (ctx_.is_static) {
                return ExecResult::STATIC_CALL_VIOLATION;
            }
            auto key = Pop();
            auto value = Pop();
            state_.SetStorage(ctx_.address, key, value);
            break;
        }
        
        // Stack operations
        case Opcode::POP:
            Pop();
            break;
        
        case Opcode::PUSH1:
        case Opcode::PUSH2:
        case Opcode::PUSH3:
        case Opcode::PUSH4:
        case Opcode::PUSH5:
        case Opcode::PUSH6:
        case Opcode::PUSH7:
        case Opcode::PUSH8:
        case Opcode::PUSH9:
        case Opcode::PUSH10:
        case Opcode::PUSH11:
        case Opcode::PUSH12:
        case Opcode::PUSH13:
        case Opcode::PUSH14:
        case Opcode::PUSH15:
        case Opcode::PUSH16:
        case Opcode::PUSH17:
        case Opcode::PUSH18:
        case Opcode::PUSH19:
        case Opcode::PUSH20:
        case Opcode::PUSH21:
        case Opcode::PUSH22:
        case Opcode::PUSH23:
        case Opcode::PUSH24:
        case Opcode::PUSH25:
        case Opcode::PUSH26:
        case Opcode::PUSH27:
        case Opcode::PUSH28:
        case Opcode::PUSH29:
        case Opcode::PUSH30:
        case Opcode::PUSH31:
        case Opcode::PUSH32: {
            uint8_t size = GetPushSize(op);
            uint256_t value{};
            for (uint8_t i = 0; i < size && pc + 1 + i < code.size(); i++) {
                value[32 - size + i] = code[pc + 1 + i];
            }
            Push(value);
            pc += size; // Skip pushed bytes
            break;
        }
        
        // DUP operations
        case Opcode::DUP1:
        case Opcode::DUP2:
        case Opcode::DUP3:
        case Opcode::DUP4:
        case Opcode::DUP5:
        case Opcode::DUP6:
        case Opcode::DUP7:
        case Opcode::DUP8:
        case Opcode::DUP9:
        case Opcode::DUP10:
        case Opcode::DUP11:
        case Opcode::DUP12:
        case Opcode::DUP13:
        case Opcode::DUP14:
        case Opcode::DUP15:
        case Opcode::DUP16:
            Dup(GetDupDepth(op));
            break;
        
        // SWAP operations
        case Opcode::SWAP1:
        case Opcode::SWAP2:
        case Opcode::SWAP3:
        case Opcode::SWAP4:
        case Opcode::SWAP5:
        case Opcode::SWAP6:
        case Opcode::SWAP7:
        case Opcode::SWAP8:
        case Opcode::SWAP9:
        case Opcode::SWAP10:
        case Opcode::SWAP11:
        case Opcode::SWAP12:
        case Opcode::SWAP13:
        case Opcode::SWAP14:
        case Opcode::SWAP15:
        case Opcode::SWAP16:
            Swap(GetSwapDepth(op));
            break;
        
        // Context operations
        case Opcode::ADDRESS:
            Push(ToUint256(0)); // Simplified - would need address conversion
            break;
        
        case Opcode::CALLER:
            Push(ToUint256(0)); // Simplified
            break;
        
        case Opcode::CALLVALUE:
            Push(ctx_.value);
            break;
        
        case Opcode::GAS:
            Push(ToUint256(GetGasRemaining()));
            break;
        
        case Opcode::GASPRICE:
            Push(ToUint256(ctx_.gas_price));
            break;
        
        case Opcode::TIMESTAMP:
            Push(ToUint256(ctx_.timestamp));
            break;
        
        case Opcode::NUMBER:
            Push(ToUint256(ctx_.block_number));
            break;
        
        case Opcode::DIFFICULTY:
            Push(ToUint256(ctx_.difficulty));
            break;
        
        case Opcode::GASLIMIT:
            Push(ToUint256(ctx_.gas_limit_block));
            break;
        
        case Opcode::CHAINID:
            Push(ToUint256(ctx_.chain_id));
            break;
        
        case Opcode::BASEFEE:
            Push(ToUint256(ctx_.base_fee));
            break;
        
        // Return operations
        case Opcode::RETURN: {
            auto offset = ToUint64(Pop());
            auto length = ToUint64(Pop());
            ExpandMemory(offset + length);
            return_data_.assign(memory_.begin() + offset, memory_.begin() + offset + length);
            return ExecResult::RETURNED;
        }
        
        case Opcode::REVERT: {
            auto offset = ToUint64(Pop());
            auto length = ToUint64(Pop());
            ExpandMemory(offset + length);
            return_data_.assign(memory_.begin() + offset, memory_.begin() + offset + length);
            return ExecResult::REVERT;
        }
        
        case Opcode::JUMPDEST:
            // Valid jump destination, just continue
            break;
        
        default:
            // Unsupported opcode
            return ExecResult::INVALID_OPCODE;
    }
    
    return ExecResult::SUCCESS; // Continue execution
}

} // namespace evm
} // namespace parthenon
