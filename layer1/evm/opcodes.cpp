// ParthenonChain - EVM Opcode Gas Costs

#include "opcodes.h"

namespace parthenon {
namespace evm {

/**
 * Gas costs for EVM opcodes (OBL-based)
 * Based on EVM gas schedule but using OBL as gas token
 */
uint64_t GetOpcodeCost(Opcode op) {
    switch (op) {
        // Zero gas
        case Opcode::STOP:
        case Opcode::INVALID:
            return 0;
        
        // Base cost (3 gas)
        case Opcode::ADDRESS:
        case Opcode::ORIGIN:
        case Opcode::CALLER:
        case Opcode::CALLVALUE:
        case Opcode::CALLDATASIZE:
        case Opcode::CODESIZE:
        case Opcode::GASPRICE:
        case Opcode::COINBASE:
        case Opcode::TIMESTAMP:
        case Opcode::NUMBER:
        case Opcode::DIFFICULTY:
        case Opcode::GASLIMIT:
        case Opcode::CHAINID:
        case Opcode::SELFBALANCE:
        case Opcode::BASEFEE:
        case Opcode::PC:
        case Opcode::MSIZE:
        case Opcode::GAS:
        case Opcode::RETURNDATASIZE:
            return 2;
        
        // Very low (3 gas)
        case Opcode::ADD:
        case Opcode::SUB:
        case Opcode::LT:
        case Opcode::GT:
        case Opcode::SLT:
        case Opcode::SGT:
        case Opcode::EQ:
        case Opcode::ISZERO:
        case Opcode::AND:
        case Opcode::OR:
        case Opcode::XOR:
        case Opcode::NOT:
        case Opcode::BYTE:
        case Opcode::SHL:
        case Opcode::SHR:
        case Opcode::SAR:
        case Opcode::POP:
        case Opcode::JUMPDEST:
            return 3;
        
        // PUSH operations (3 gas)
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
        case Opcode::PUSH32:
            return 3;
        
        // DUP operations (3 gas)
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
            return 3;
        
        // SWAP operations (3 gas)
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
            return 3;
        
        // Low (5 gas)
        case Opcode::MUL:
        case Opcode::DIV:
        case Opcode::SDIV:
        case Opcode::MOD:
        case Opcode::SMOD:
        case Opcode::SIGNEXTEND:
            return 5;
        
        // Mid (8 gas)
        case Opcode::ADDMOD:
        case Opcode::MULMOD:
        case Opcode::JUMP:
            return 8;
        
        // High (10 gas)
        case Opcode::JUMPI:
        case Opcode::EXP: // Base cost, additional per byte
            return 10;
        
        // Memory operations
        case Opcode::MLOAD:
        case Opcode::MSTORE:
        case Opcode::MSTORE8:
            return 3;
        
        // Storage operations
        case Opcode::SLOAD:
            return 800; // Expensive
        case Opcode::SSTORE:
            return 20000; // Very expensive base cost
        
        // Copy operations
        case Opcode::CALLDATALOAD:
            return 3;
        case Opcode::CALLDATACOPY:
        case Opcode::CODECOPY:
        case Opcode::RETURNDATACOPY:
            return 3; // Base cost, additional per word
        case Opcode::EXTCODECOPY:
            return 700; // Base cost, additional per word
        
        // External operations
        case Opcode::BALANCE:
            return 700;
        case Opcode::EXTCODESIZE:
        case Opcode::EXTCODEHASH:
            return 700;
        case Opcode::BLOCKHASH:
            return 20;
        
        // SHA3
        case Opcode::SHA3:
            return 30; // Base cost, additional per word
        
        // Logging
        case Opcode::LOG0:
        case Opcode::LOG1:
        case Opcode::LOG2:
        case Opcode::LOG3:
        case Opcode::LOG4:
            return 375; // Base cost, additional per byte and topic
        
        // Contract operations
        case Opcode::CREATE:
        case Opcode::CREATE2:
            return 32000;
        case Opcode::CALL:
        case Opcode::CALLCODE:
        case Opcode::DELEGATECALL:
        case Opcode::STATICCALL:
            return 700; // Base cost, additional if value transfer
        
        // Return operations
        case Opcode::RETURN:
        case Opcode::REVERT:
            return 0; // Gas refund handled separately
        
        // Selfdestruct
        case Opcode::SELFDESTRUCT:
            return 5000; // Base cost, additional if beneficiary account created
        
        default:
            return 0;
    }
}

} // namespace evm
} // namespace parthenon
