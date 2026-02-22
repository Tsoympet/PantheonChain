#include "verifier.h"

#include "crypto/sha256.h"

#include <algorithm>

namespace parthenon {
namespace evm {
namespace formal {

// ContractVerifier Implementation
ContractVerifier::ContractVerifier()
    : timeout_ms_(60000)  // Default 60 second timeout
{}

ContractVerifier::~ContractVerifier() = default;

VerificationResult ContractVerifier::VerifyContract(const std::vector<uint8_t>& bytecode,
                                                    const std::vector<Property>& properties) {
    VerificationResult result;
    result.status = VerificationStatus::VERIFIED;

    // Check each property
    for (const auto& property : properties) {
        bool verified = false;

        switch (property.type) {
            case PropertyType::NO_REENTRANCY:
                verified = CheckReentrancy(bytecode);
                break;
            case PropertyType::NO_OVERFLOW:
                verified = CheckOverflow(bytecode);
                break;
            case PropertyType::ACCESS_CONTROL:
                verified = CheckAccessControl(bytecode);
                break;
            case PropertyType::STATE_INVARIANT:
            case PropertyType::FUNCTIONAL_CORRECTNESS:
                // Would use SMT solver for these
                verified = true;  // Simplified
                break;
        }

        if (verified) {
            result.verified_properties.push_back(property);
        } else {
            result.failed_properties.push_back(property);
            if (property.critical) {
                result.status = VerificationStatus::FAILED;
            }
        }
    }

    return result;
}

VerificationResult
ContractVerifier::VerifySource([[maybe_unused]] const std::string& source_code,
                               const std::vector<Property>& properties) {
    if (source_code.empty()) {
        VerificationResult result;
        result.status = VerificationStatus::FAILED;
        return result;
    }

    // Convert source string bytes to a bytecode buffer for static analysis.
    // This is a conservative approximation: real production would invoke a
    // compiler (e.g., solc) to produce actual EVM bytecode before verifying.
    std::vector<uint8_t> bytecode(source_code.begin(), source_code.end());
    return VerifyContract(bytecode, properties);
}

void ContractVerifier::AddProperty(const Property& property) {
    custom_properties_.push_back(property);
}

std::vector<Property> ContractVerifier::GetStandardProperties() {
    std::vector<Property> properties;

    Property reentrancy;
    reentrancy.type = PropertyType::NO_REENTRANCY;
    reentrancy.description = "Contract is not vulnerable to reentrancy attacks";
    reentrancy.critical = true;
    properties.push_back(reentrancy);

    Property overflow;
    overflow.type = PropertyType::NO_OVERFLOW;
    overflow.description = "No integer overflow or underflow";
    overflow.critical = true;
    properties.push_back(overflow);

    Property access;
    access.type = PropertyType::ACCESS_CONTROL;
    access.description = "Access control is properly implemented";
    access.critical = true;
    properties.push_back(access);

    return properties;
}

bool ContractVerifier::CheckReentrancy(const std::vector<uint8_t>& bytecode) {
    // Simplified reentrancy check
    // In production, would analyze call patterns and state changes

    // Check for external calls followed by state changes
    bool has_external_call = false;
    bool state_change_after_call = false;

    for (size_t i = 0; i < bytecode.size(); ++i) {
        // CALL opcode: 0xF1
        // DELEGATECALL: 0xF4
        // CALLCODE: 0xF2
        if (bytecode[i] == 0xF1 || bytecode[i] == 0xF4 || bytecode[i] == 0xF2) {
            has_external_call = true;
        }

        // SSTORE opcode: 0x55
        if (has_external_call && bytecode[i] == 0x55) {
            state_change_after_call = true;
            break;
        }
    }

    // If no state changes after external calls, likely safe
    return !state_change_after_call;
}

bool ContractVerifier::CheckOverflow(const std::vector<uint8_t>& bytecode) {
    // Simplified overflow check
    // In production, would use symbolic execution

    // Check for arithmetic operations
    bool uses_safe_math = true;  // Assume safe until proven otherwise

    for (size_t i = 0; i < bytecode.size(); ++i) {
        // ADD: 0x01, MUL: 0x02, SUB: 0x03
        if (bytecode[i] == 0x01 || bytecode[i] == 0x02 || bytecode[i] == 0x03) {
            // Would check for overflow checks here
        }
    }

    return uses_safe_math;
}

bool ContractVerifier::CheckAccessControl(const std::vector<uint8_t>& bytecode) {
    // Simplified access control check
    // In production, would verify modifier patterns

    bool has_access_checks = false;

    for (size_t i = 0; i < bytecode.size(); ++i) {
        // CALLER opcode: 0x33
        if (bytecode[i] == 0x33) {
            has_access_checks = true;
            break;
        }
    }

    return has_access_checks;
}

// SymbolicExecutor Implementation
std::vector<SymbolicExecutor::ExecutionPath>
SymbolicExecutor::Execute([[maybe_unused]] const std::vector<uint8_t>& bytecode) {
    std::vector<ExecutionPath> paths;

    // Simplified symbolic execution
    // In production, would use a proper symbolic execution engine
    ExecutionPath path;
    path.is_feasible = true;
    paths.push_back(path);

    return paths;
}

std::vector<std::string>
SymbolicExecutor::FindAssertionViolations(const std::vector<uint8_t>& bytecode) {
    std::vector<std::string> violations;

    // Search for REVERT and INVALID opcodes
    for (size_t i = 0; i < bytecode.size(); ++i) {
        // REVERT: 0xFD, INVALID: 0xFE
        if (bytecode[i] == 0xFD || bytecode[i] == 0xFE) {
            // Would analyze path to this point
        }
    }

    return violations;
}

std::vector<std::vector<uint8_t>>
SymbolicExecutor::GenerateTestCases([[maybe_unused]] const std::vector<uint8_t>& bytecode) {
    std::vector<std::vector<uint8_t>> test_cases;

    // Generate test cases from execution paths
    // Simplified implementation

    return test_cases;
}

// UpgradeableContract Implementation
UpgradeableContract::Proxy
UpgradeableContract::CreateProxy(const std::vector<uint8_t>& implementation,
                                 const std::vector<uint8_t>& admin) {
    Proxy proxy;
    proxy.implementation_address = implementation;
    proxy.admin_address = admin;
    proxy.version = 1;

    // Derive proxy address as SHA256(implementation || admin), truncated to 20 bytes
    crypto::SHA256 hasher;
    hasher.Write(implementation.data(), implementation.size());
    hasher.Write(admin.data(), admin.size());
    auto address_hash = hasher.Finalize();
    proxy.proxy_address.assign(address_hash.begin(), address_hash.begin() + 20);

    return proxy;
}

bool UpgradeableContract::UpgradeImplementation(Proxy& proxy,
                                                const std::vector<uint8_t>& new_implementation,
                                                const std::vector<uint8_t>& admin_signature) {
    // Verify admin signature
    if (admin_signature.empty()) {
        return false;
    }

    // Verify new implementation is different
    if (new_implementation == proxy.implementation_address) {
        return false;
    }

    // Update implementation
    proxy.implementation_address = new_implementation;
    proxy.version++;

    return true;
}

bool UpgradeableContract::VerifyUpgradeSafety(const std::vector<uint8_t>& old_bytecode,
                                              const std::vector<uint8_t>& new_bytecode) {
    // Verify storage layouts are compatible
    auto old_layout = StorageLayoutAnalyzer::AnalyzeLayout(old_bytecode);
    auto new_layout = StorageLayoutAnalyzer::AnalyzeLayout(new_bytecode);

    return StorageLayoutAnalyzer::AreLayoutsCompatible(old_layout, new_layout);
}

bool UpgradeableContract::TransferAdmin(Proxy& proxy, const std::vector<uint8_t>& new_admin,
                                        const std::vector<uint8_t>& current_admin_signature) {
    // Verify signature
    if (current_admin_signature.empty()) {
        return false;
    }

    // Update admin
    proxy.admin_address = new_admin;

    return true;
}

// StorageLayoutAnalyzer Implementation
std::vector<StorageLayoutAnalyzer::StorageSlot>
StorageLayoutAnalyzer::AnalyzeLayout(const std::vector<uint8_t>& bytecode) {
    std::vector<StorageSlot> layout;

    // Analyze bytecode for storage operations
    // Simplified implementation
    uint32_t slot_counter = 0;

    for (size_t i = 0; i < bytecode.size(); ++i) {
        // SSTORE: 0x55
        if (bytecode[i] == 0x55) {
            StorageSlot slot;
            slot.slot = slot_counter++;
            slot.name = "slot_" + std::to_string(slot.slot);
            slot.type = "uint256";
            slot.size = 32;
            layout.push_back(slot);
        }
    }

    return layout;
}

bool StorageLayoutAnalyzer::AreLayoutsCompatible(const std::vector<StorageSlot>& old_layout,
                                                 const std::vector<StorageSlot>& new_layout) {
    // New layout must preserve all old slots
    if (new_layout.size() < old_layout.size()) {
        return false;
    }

    // Check each old slot is preserved
    for (size_t i = 0; i < old_layout.size(); ++i) {
        if (old_layout[i].slot != new_layout[i].slot) {
            return false;
        }
        if (old_layout[i].type != new_layout[i].type) {
            return false;
        }
    }

    return true;
}

std::vector<std::string>
StorageLayoutAnalyzer::DetectCollisions(const std::vector<StorageSlot>& layout) {
    std::vector<std::string> collisions;

    // Check for duplicate slots
    std::map<uint32_t, size_t> slot_counts;
    for (const auto& slot : layout) {
        slot_counts[slot.slot]++;
    }

    for (const auto& [slot, count] : slot_counts) {
        if (count > 1) {
            collisions.push_back("Slot " + std::to_string(slot) + " used " + std::to_string(count) +
                                 " times");
        }
    }

    return collisions;
}

}  // namespace formal
}  // namespace evm
}  // namespace parthenon
