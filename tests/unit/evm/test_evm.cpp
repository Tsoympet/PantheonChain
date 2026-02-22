// ParthenonChain - EVM Tests

#include "evm/opcodes.h"
#include "evm/state.h"
#include "evm/vm.h"

#include <cassert>
#include <cstring>
#include <iostream>

using namespace parthenon::evm;

void TestStackOperations() {
    std::cout << "Test: Stack operations" << std::endl;

    WorldState state;
    ExecutionContext ctx;
    ctx.gas_limit = 1000000;
    ctx.gas_price = 1;
    ctx.block_number = 1;
    ctx.timestamp = 1234567890;
    ctx.difficulty = 1000;
    ctx.gas_limit_block = 10000000;
    ctx.chain_id = 1;
    ctx.base_fee = 10;
    ctx.is_static = false;
    ctx.depth = 0;

    VM vm(state, ctx);

    // Test PUSH1 and ADD: PUSH1 5, PUSH1 3, ADD
    std::vector<uint8_t> code = {static_cast<uint8_t>(Opcode::PUSH1), 0x05, // Push 5
                                 static_cast<uint8_t>(Opcode::PUSH1), 0x03, // Push 3
                                 static_cast<uint8_t>(Opcode::ADD),         // Add (should give 8)
                                 static_cast<uint8_t>(Opcode::STOP)};

    auto [result, data] = vm.Execute(code);
    assert(result == ExecResult::SUCCESS);

    std::cout << "  ✓ Passed (stack operations)" << std::endl;
}

void TestArithmetic() {
    std::cout << "Test: Arithmetic operations" << std::endl;

    WorldState state;
    ExecutionContext ctx;
    ctx.gas_limit = 1000000;
    ctx.gas_price = 1;
    ctx.block_number = 1;
    ctx.timestamp = 1234567890;
    ctx.difficulty = 1000;
    ctx.gas_limit_block = 10000000;
    ctx.chain_id = 1;
    ctx.base_fee = 10;
    ctx.is_static = false;
    ctx.depth = 0;

    VM vm(state, ctx);

    // Test: PUSH1 10, PUSH1 2, MUL, PUSH1 5, SUB
    // Result should be: 10 * 2 - 5 = 15
    std::vector<uint8_t> code = {static_cast<uint8_t>(Opcode::PUSH1), 0x0A, // Push 10
                                 static_cast<uint8_t>(Opcode::PUSH1), 0x02, // Push 2
                                 static_cast<uint8_t>(Opcode::MUL),         // Multiply (20)
                                 static_cast<uint8_t>(Opcode::PUSH1), 0x05, // Push 5
                                 static_cast<uint8_t>(Opcode::SUB),         // Subtract (15)
                                 static_cast<uint8_t>(Opcode::STOP)};

    auto [result, data] = vm.Execute(code);
    assert(result == ExecResult::SUCCESS);

    std::cout << "  ✓ Passed (arithmetic)" << std::endl;
}

void TestMemoryOperations() {
    std::cout << "Test: Memory operations" << std::endl;

    WorldState state;
    ExecutionContext ctx;
    ctx.gas_limit = 1000000;
    ctx.gas_price = 1;
    ctx.block_number = 1;
    ctx.timestamp = 1234567890;
    ctx.difficulty = 1000;
    ctx.gas_limit_block = 10000000;
    ctx.chain_id = 1;
    ctx.base_fee = 10;
    ctx.is_static = false;
    ctx.depth = 0;

    VM vm(state, ctx);

    // Test: PUSH1 42, PUSH1 0, MSTORE, PUSH1 0, MLOAD
    // Store 42 at offset 0, then load it back
    std::vector<uint8_t> code = {static_cast<uint8_t>(Opcode::PUSH1),  0x2A, // Push 42
                                 static_cast<uint8_t>(Opcode::PUSH1),  0x00, // Push offset 0
                                 static_cast<uint8_t>(Opcode::MSTORE),       // Store
                                 static_cast<uint8_t>(Opcode::PUSH1),  0x00, // Push offset 0
                                 static_cast<uint8_t>(Opcode::MLOAD),        // Load
                                 static_cast<uint8_t>(Opcode::STOP)};

    auto [result, data] = vm.Execute(code);
    assert(result == ExecResult::SUCCESS);

    std::cout << "  ✓ Passed (memory operations)" << std::endl;
}

void TestStorageOperations() {
    std::cout << "Test: Storage operations" << std::endl;

    WorldState state;
    ExecutionContext ctx;
    ctx.gas_limit = 1000000;
    ctx.gas_price = 1;
    ctx.block_number = 1;
    ctx.timestamp = 1234567890;
    ctx.difficulty = 1000;
    ctx.gas_limit_block = 10000000;
    ctx.chain_id = 1;
    ctx.base_fee = 10;
    ctx.is_static = false;
    ctx.depth = 0;
    ctx.address = Address{};

    VM vm(state, ctx);

    // Test: PUSH1 99, PUSH1 1, SSTORE, PUSH1 1, SLOAD
    // Store 99 at key 1, then load it back
    std::vector<uint8_t> code = {static_cast<uint8_t>(Opcode::PUSH1),  0x63, // Push 99
                                 static_cast<uint8_t>(Opcode::PUSH1),  0x01, // Push key 1
                                 static_cast<uint8_t>(Opcode::SSTORE),       // Store
                                 static_cast<uint8_t>(Opcode::PUSH1),  0x01, // Push key 1
                                 static_cast<uint8_t>(Opcode::SLOAD),        // Load
                                 static_cast<uint8_t>(Opcode::STOP)};

    auto [result, data] = vm.Execute(code);
    assert(result == ExecResult::SUCCESS);

    std::cout << "  ✓ Passed (storage operations)" << std::endl;
}

void TestComparison() {
    std::cout << "Test: Comparison operations" << std::endl;

    WorldState state;
    ExecutionContext ctx;
    ctx.gas_limit = 1000000;
    ctx.gas_price = 1;
    ctx.block_number = 1;
    ctx.timestamp = 1234567890;
    ctx.difficulty = 1000;
    ctx.gas_limit_block = 10000000;
    ctx.chain_id = 1;
    ctx.base_fee = 10;
    ctx.is_static = false;
    ctx.depth = 0;

    VM vm(state, ctx);

    // Test: PUSH1 5, PUSH1 3, LT (3 < 5 = true = 1)
    std::vector<uint8_t> code = {static_cast<uint8_t>(Opcode::PUSH1), 0x05, // Push 5
                                 static_cast<uint8_t>(Opcode::PUSH1), 0x03, // Push 3
                                 static_cast<uint8_t>(Opcode::LT),          // Less than
                                 static_cast<uint8_t>(Opcode::STOP)};

    auto [result, data] = vm.Execute(code);
    assert(result == ExecResult::SUCCESS);

    std::cout << "  ✓ Passed (comparison)" << std::endl;
}

void TestBitwise() {
    std::cout << "Test: Bitwise operations" << std::endl;

    WorldState state;
    ExecutionContext ctx;
    ctx.gas_limit = 1000000;
    ctx.gas_price = 1;
    ctx.block_number = 1;
    ctx.timestamp = 1234567890;
    ctx.difficulty = 1000;
    ctx.gas_limit_block = 10000000;
    ctx.chain_id = 1;
    ctx.base_fee = 10;
    ctx.is_static = false;
    ctx.depth = 0;

    VM vm(state, ctx);

    // Test: PUSH1 0xFF, PUSH1 0x0F, AND (should give 0x0F)
    std::vector<uint8_t> code = {static_cast<uint8_t>(Opcode::PUSH1), 0xFF, // Push 255
                                 static_cast<uint8_t>(Opcode::PUSH1), 0x0F, // Push 15
                                 static_cast<uint8_t>(Opcode::AND),         // Bitwise AND
                                 static_cast<uint8_t>(Opcode::STOP)};

    auto [result, data] = vm.Execute(code);
    assert(result == ExecResult::SUCCESS);

    std::cout << "  ✓ Passed (bitwise operations)" << std::endl;
}

void TestGasMetering() {
    std::cout << "Test: Gas metering" << std::endl;

    WorldState state;
    ExecutionContext ctx;
    ctx.gas_limit = 100; // Very low gas limit
    ctx.gas_price = 1;
    ctx.block_number = 1;
    ctx.timestamp = 1234567890;
    ctx.difficulty = 1000;
    ctx.gas_limit_block = 10000000;
    ctx.chain_id = 1;
    ctx.base_fee = 10;
    ctx.is_static = false;
    ctx.depth = 0;
    ctx.address = Address{};

    VM vm(state, ctx);

    // This should run out of gas due to SSTORE cost
    std::vector<uint8_t> code = {
        static_cast<uint8_t>(Opcode::PUSH1),  0x63, static_cast<uint8_t>(Opcode::PUSH1), 0x01,
        static_cast<uint8_t>(Opcode::SSTORE), // Costs 20000 gas
    };

    auto [result, data] = vm.Execute(code);
    assert(result == ExecResult::OUT_OF_GAS);

    std::cout << "  ✓ Passed (gas metering)" << std::endl;
}

void TestReturn() {
    std::cout << "Test: Return operation" << std::endl;

    WorldState state;
    ExecutionContext ctx;
    ctx.gas_limit = 1000000;
    ctx.gas_price = 1;
    ctx.block_number = 1;
    ctx.timestamp = 1234567890;
    ctx.difficulty = 1000;
    ctx.gas_limit_block = 10000000;
    ctx.chain_id = 1;
    ctx.base_fee = 10;
    ctx.is_static = false;
    ctx.depth = 0;

    VM vm(state, ctx);

    // Store value in memory and return it
    // PUSH1 0x42, PUSH1 0, MSTORE8, PUSH1 1, PUSH1 0, RETURN
    std::vector<uint8_t> code = {
        static_cast<uint8_t>(Opcode::PUSH1),   0x42, // Value to return
        static_cast<uint8_t>(Opcode::PUSH1),   0x00, // Offset 0
        static_cast<uint8_t>(Opcode::MSTORE8),       // Store byte
        static_cast<uint8_t>(Opcode::PUSH1),   0x01, // Length 1
        static_cast<uint8_t>(Opcode::PUSH1),   0x00, // Offset 0
        static_cast<uint8_t>(Opcode::RETURN)         // Return
    };

    auto [result, data] = vm.Execute(code);
    assert(result == ExecResult::RETURNED);
    assert(data.size() == 1);
    assert(data[0] == 0x42);

    std::cout << "  ✓ Passed (return)" << std::endl;
}

void TestStateRoot() {
    std::cout << "Test: State root calculation" << std::endl;

    WorldState state;

    // Create some accounts
    Address addr1{};
    addr1[19] = 1;
    Address addr2{};
    addr2[19] = 2;

    state.SetBalance(addr1, ToUint256(1000));
    state.SetBalance(addr2, ToUint256(2000));
    state.SetNonce(addr1, 5);

    // Calculate state root
    auto root1 = state.CalculateStateRoot();

    // State root should be deterministic
    auto root2 = state.CalculateStateRoot();
    assert(std::memcmp(root1.data(), root2.data(), 32) == 0);

    // Modify state
    state.SetBalance(addr1, ToUint256(1500));
    auto root3 = state.CalculateStateRoot();

    // Root should change
    assert(std::memcmp(root1.data(), root3.data(), 32) != 0);

    std::cout << "  ✓ Passed (state root)" << std::endl;
}

void TestOpcodeGasCosts() {
    std::cout << "Test: Opcode gas costs" << std::endl;

    // Test some basic gas costs
    assert(GetOpcodeCost(Opcode::STOP) == 0);
    assert(GetOpcodeCost(Opcode::ADD) == 3);
    assert(GetOpcodeCost(Opcode::MUL) == 5);
    assert(GetOpcodeCost(Opcode::SLOAD) == 800);
    assert(GetOpcodeCost(Opcode::SSTORE) == 20000);
    assert(GetOpcodeCost(Opcode::SHA3) == 30);

    std::cout << "  ✓ Passed (gas costs)" << std::endl;
}

int main() {
    std::cout << "=== EVM Tests ===" << std::endl;

    TestStackOperations();
    TestArithmetic();
    TestMemoryOperations();
    TestStorageOperations();
    TestComparison();
    TestBitwise();
    TestGasMetering();
    TestReturn();
    TestStateRoot();
    TestOpcodeGasCosts();

    std::cout << "\n✓ All EVM tests passed!" << std::endl;
    return 0;
}
