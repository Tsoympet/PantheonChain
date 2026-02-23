#include "destination_tag.h"


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

    // Validate strict UTF-8 byte sequences.
    for (size_t i = 0; i < memo.size();) {
        const uint8_t c = static_cast<uint8_t>(memo[i]);

        if (c <= 0x7F) {
            ++i;
            continue;
        }

        size_t len = 0;
        uint32_t codepoint = 0;

        if ((c & 0xE0) == 0xC0) {
            len = 2;
            codepoint = c & 0x1F;
            if (codepoint == 0) {
                return false;  // Overlong encoding.
            }
        } else if ((c & 0xF0) == 0xE0) {
            len = 3;
            codepoint = c & 0x0F;
        } else if ((c & 0xF8) == 0xF0) {
            len = 4;
            codepoint = c & 0x07;
        } else {
            return false;
        }

        if (i + len > memo.size()) {
            return false;
        }

        for (size_t j = 1; j < len; ++j) {
            const uint8_t cont = static_cast<uint8_t>(memo[i + j]);
            if ((cont & 0xC0) != 0x80) {
                return false;
            }
            codepoint = (codepoint << 6) | (cont & 0x3F);
        }

        // Reject overlong forms, UTF-16 surrogate range and values above Unicode max.
        if ((len == 2 && codepoint < 0x80) || (len == 3 && codepoint < 0x800) ||
            (len == 4 && codepoint < 0x10000) || codepoint > 0x10FFFF ||
            (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
            return false;
        }

        i += len;
    }

    return true;
}

bool TagValidator::ValidateDestinationTag(const DestinationTag& dt) {
    return ValidateTag(dt.GetTag()) && ValidateMemo(dt.GetMemo());
}

}  // namespace settlement
}  // namespace parthenon
