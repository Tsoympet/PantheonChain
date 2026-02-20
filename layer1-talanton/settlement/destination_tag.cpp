#include "destination_tag.h"

#include <cstring>

namespace parthenon {
namespace settlement {

DestinationTag::DestinationTag() : tag_(0), memo_() {}

DestinationTag::DestinationTag(uint32_t tag) : tag_(tag), memo_() {}

DestinationTag::DestinationTag(uint32_t tag, const std::string& memo) : tag_(tag), memo_(memo) {}

bool DestinationTag::IsValid() const {
    return TagValidator::ValidateDestinationTag(*this);
}

std::vector<uint8_t> DestinationTag::Serialize() const {
    std::vector<uint8_t> result;

    // Serialize tag (4 bytes, little-endian)
    result.push_back(static_cast<uint8_t>(tag_ & 0xFF));
    result.push_back(static_cast<uint8_t>((tag_ >> 8) & 0xFF));
    result.push_back(static_cast<uint8_t>((tag_ >> 16) & 0xFF));
    result.push_back(static_cast<uint8_t>((tag_ >> 24) & 0xFF));

    // Serialize memo length (2 bytes, little-endian)
    uint16_t memo_len = static_cast<uint16_t>(memo_.size());
    result.push_back(static_cast<uint8_t>(memo_len & 0xFF));
    result.push_back(static_cast<uint8_t>((memo_len >> 8) & 0xFF));

    // Serialize memo data
    result.insert(result.end(), memo_.begin(), memo_.end());

    return result;
}

DestinationTag DestinationTag::Deserialize(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos + 6 > data.size()) {
        return DestinationTag();
    }

    // Deserialize tag
    uint32_t tag = static_cast<uint32_t>(data[pos]) | (static_cast<uint32_t>(data[pos + 1]) << 8) |
                   (static_cast<uint32_t>(data[pos + 2]) << 16) |
                   (static_cast<uint32_t>(data[pos + 3]) << 24);
    pos += 4;

    // Deserialize memo length
    uint16_t memo_len =
        static_cast<uint16_t>(data[pos]) | (static_cast<uint16_t>(data[pos + 1]) << 8);
    pos += 2;

    // Check bounds
    if (pos + memo_len > data.size()) {
        return DestinationTag();
    }

    // Deserialize memo
    std::string memo(data.begin() + pos, data.begin() + pos + memo_len);
    pos += memo_len;

    return DestinationTag(tag, memo);
}

bool DestinationTag::operator==(const DestinationTag& other) const {
    return tag_ == other.tag_ && memo_ == other.memo_;
}

bool DestinationTag::operator!=(const DestinationTag& other) const {
    return !(*this == other);
}

bool TagValidator::ValidateTag(uint32_t /* tag */) {
    // All uint32_t values are valid tags
    return true;
}

bool TagValidator::ValidateMemo(const std::string& memo) {
    // Check memo size
    if (memo.size() > DestinationTag::MAX_MEMO_SIZE) {
        return false;
    }

    // Check for valid UTF-8 characters (simplified: printable ASCII)
    for (char c : memo) {
        if (c < 32 || c > 126) {
            if (c != '\n' && c != '\r' && c != '\t') {
                return false;
            }
        }
    }

    return true;
}

bool TagValidator::ValidateDestinationTag(const DestinationTag& dt) {
    return ValidateTag(dt.GetTag()) && ValidateMemo(dt.GetMemo());
}

}  // namespace settlement
}  // namespace parthenon
