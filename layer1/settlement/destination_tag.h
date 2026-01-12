#ifndef PARTHENON_SETTLEMENT_DESTINATION_TAG_H
#define PARTHENON_SETTLEMENT_DESTINATION_TAG_H

#include <cstdint>
#include <string>
#include <vector>

namespace parthenon {
namespace settlement {

// Destination tags allow routing DRM payments to specific sub-accounts
// or purposes without requiring separate addresses
class DestinationTag {
public:
    static constexpr uint32_t MAX_TAG_VALUE = 0xFFFFFFFF;
    static constexpr size_t MAX_MEMO_SIZE = 256;

    DestinationTag();
    explicit DestinationTag(uint32_t tag);
    DestinationTag(uint32_t tag, const std::string& memo);

    uint32_t GetTag() const { return tag_; }
    const std::string& GetMemo() const { return memo_; }
    
    bool IsValid() const;
    
    // Serialization
    std::vector<uint8_t> Serialize() const;
    static DestinationTag Deserialize(const std::vector<uint8_t>& data, size_t& pos);
    
    bool operator==(const DestinationTag& other) const;
    bool operator!=(const DestinationTag& other) const;

private:
    uint32_t tag_;
    std::string memo_;
};

// Tag validator ensures tags are properly formatted and within limits
class TagValidator {
public:
    static bool ValidateTag(uint32_t tag);
    static bool ValidateMemo(const std::string& memo);
    static bool ValidateDestinationTag(const DestinationTag& dt);
};

} // namespace settlement
} // namespace parthenon

#endif // PARTHENON_SETTLEMENT_DESTINATION_TAG_H
