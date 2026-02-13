#ifndef NLOHMANN_JSON_STUB_HPP
#define NLOHMANN_JSON_STUB_HPP

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <initializer_list>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace nlohmann {

class json {
  public:
    using object_t = std::map<std::string, json>;
    using array_t = std::vector<json>;

    json() : type_(Type::kNull), boolean_(false), number_(0.0) {}
    json(std::nullptr_t) : json() {}
    json(bool value) : type_(Type::kBoolean), boolean_(value), number_(0.0) {}

    template <typename Integer,
              typename std::enable_if_t<std::is_integral_v<Integer> &&
                                            !std::is_same_v<Integer, bool>,
                                        int> = 0>
    json(Integer value) : type_(Type::kNumber), boolean_(false), number_(static_cast<double>(value)) {}

    json(double value) : type_(Type::kNumber), boolean_(false), number_(value) {}
    json(const char* value) : type_(Type::kString), boolean_(false), number_(0.0), string_(value ? value : "") {}
    json(const std::string& value) : type_(Type::kString), boolean_(false), number_(0.0), string_(value) {}
    json(std::string&& value)
        : type_(Type::kString), boolean_(false), number_(0.0), string_(std::move(value)) {}
    json(const object_t& value) : type_(Type::kObject), boolean_(false), number_(0.0), object_(value) {}
    json(object_t&& value)
        : type_(Type::kObject), boolean_(false), number_(0.0), object_(std::move(value)) {}
    json(const array_t& value) : type_(Type::kArray), boolean_(false), number_(0.0), array_(value) {}
    json(array_t&& value)
        : type_(Type::kArray), boolean_(false), number_(0.0), array_(std::move(value)) {}

    json(std::initializer_list<std::pair<const std::string, json>> init)
        : type_(Type::kObject), boolean_(false), number_(0.0), object_(init.begin(), init.end()) {}

    static json parse(const std::string& input) {
        size_t pos = 0;
        json value = ParseValue(input, pos);
        SkipWhitespace(input, pos);
        if (pos != input.size()) {
            throw std::runtime_error("json stub parser failed to parse input");
        }
        return value;
    }

    static json parse(const std::string& input, std::nullptr_t, bool) { return parse(input); }

    static json array() { return json(array_t{}); }
    static json array(std::initializer_list<json> init) { return json(array_t(init)); }

    bool is_array() const { return type_ == Type::kArray; }
    bool is_object() const { return type_ == Type::kObject; }
    bool is_number() const { return type_ == Type::kNumber; }
    bool is_string() const { return type_ == Type::kString; }
    bool is_boolean() const { return type_ == Type::kBoolean; }
    bool is_null() const { return type_ == Type::kNull; }
    bool is_number_unsigned() const { return type_ == Type::kNumber && number_ >= 0; }
    bool is_discarded() const { return false; }

    bool empty() const {
        if (is_array()) {
            return array_.empty();
        }
        if (is_object()) {
            return object_.empty();
        }
        return true;
    }

    size_t size() const {
        if (is_array()) {
            return array_.size();
        }
        if (is_object()) {
            return object_.size();
        }
        return 0;
    }

    bool contains(const std::string& key) const {
        if (!is_object()) {
            return false;
        }
        return object_.find(key) != object_.end();
    }

    template <typename T>
    T get() const {
        return GetValue<T>();
    }

    template <typename T>
    T value(const std::string& key, const T& default_value) const {
        if (!contains(key)) {
            return default_value;
        }
        return (*this)[key].get<T>();
    }

    std::string value(const std::string& key, const char* default_value) const {
        return value<std::string>(key, default_value ? std::string(default_value) : std::string());
    }

    json& operator[](const std::string& key) {
        EnsureObject();
        return object_[key];
    }

    const json& operator[](const std::string& key) const {
        if (!contains(key)) {
            return Null();
        }
        return object_.at(key);
    }

    json& operator[](size_t index) {
        EnsureArray();
        if (index >= array_.size()) {
            array_.resize(index + 1);
        }
        return array_[index];
    }

    const json& operator[](size_t index) const {
        if (!is_array() || index >= array_.size()) {
            return Null();
        }
        return array_[index];
    }

    array_t::iterator begin() { return array_.begin(); }
    array_t::iterator end() { return array_.end(); }
    array_t::const_iterator begin() const { return array_.begin(); }
    array_t::const_iterator end() const { return array_.end(); }

    void push_back(const json& value) {
        EnsureArray();
        array_.push_back(value);
    }

    void push_back(json&& value) {
        EnsureArray();
        array_.push_back(std::move(value));
    }

    std::string dump() const {
        switch (type_) {
            case Type::kString:
                return "\"" + string_ + "\"";
            case Type::kNumber:
                return std::to_string(number_);
            case Type::kBoolean:
                return boolean_ ? "true" : "false";
            case Type::kArray:
                return DumpArray();
            case Type::kObject:
                return DumpObject();
            case Type::kNull:
            default:
                return "null";
        }
    }

  private:
    enum class Type { kNull, kBoolean, kNumber, kString, kArray, kObject };

    template <typename T>
    T GetValue() const {
        if constexpr (std::is_same_v<T, std::string>) {
            if (type_ == Type::kString) {
                return string_;
            }
            if (type_ == Type::kNumber) {
                return std::to_string(number_);
            }
            if (type_ == Type::kBoolean) {
                return boolean_ ? "true" : "false";
            }
            return std::string();
        } else if constexpr (std::is_integral_v<T>) {
            if (type_ == Type::kNumber) {
                return static_cast<T>(number_);
            }
            if (type_ == Type::kString) {
                try {
                    return static_cast<T>(std::stoll(string_));
                } catch (...) {
                    return T{};
                }
            }
            return T{};
        } else if constexpr (std::is_floating_point_v<T>) {
            if (type_ == Type::kNumber) {
                return static_cast<T>(number_);
            }
            if (type_ == Type::kString) {
                try {
                    return static_cast<T>(std::stod(string_));
                } catch (...) {
                    return T{};
                }
            }
            return T{};
        } else {
            return T{};
        }
    }

    void EnsureArray() {
        if (!is_array()) {
            type_ = Type::kArray;
            array_.clear();
        }
    }

    void EnsureObject() {
        if (!is_object()) {
            type_ = Type::kObject;
            object_.clear();
        }
    }

    static const json& Null() {
        static json null_json;
        return null_json;
    }

    std::string DumpArray() const {
        std::string output = "[";
        for (size_t i = 0; i < array_.size(); ++i) {
            if (i > 0) {
                output += ",";
            }
            output += array_[i].dump();
        }
        output += "]";
        return output;
    }

    std::string DumpObject() const {
        std::string output = "{";
        bool first = true;
        for (const auto& item : object_) {
            if (!first) {
                output += ",";
            }
            output += "\"" + item.first + "\":" + item.second.dump();
            first = false;
        }
        output += "}";
        return output;
    }

    static std::string Trim(const std::string& value) {
        size_t start = 0;
        size_t end = value.size();
        while (start < end && std::isspace(static_cast<unsigned char>(value[start]))) {
            ++start;
        }
        while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
            // end > start ensures end - 1 is a valid index.
            --end;
        }
        return value.substr(start, end - start);
    }

    static bool TryParseNumber(const std::string& value, double* out) {
        if (value.empty()) {
            return false;
        }
        char* end = nullptr;
        const char* start = value.c_str();
        errno = 0;
        double result = std::strtod(start, &end);
        if (start == end || *end != '\0' || errno == ERANGE) {
            return false;
        }
        if (out != nullptr) {
            *out = result;
        }
        return true;
    }

    static void SkipWhitespace(const std::string& value, size_t& pos) {
        while (pos < value.size() && std::isspace(static_cast<unsigned char>(value[pos]))) {
            ++pos;
        }
    }

    static json ParseValue(const std::string& value, size_t& pos) {
        SkipWhitespace(value, pos);
        if (pos >= value.size()) {
            throw std::runtime_error("json stub parser failed to parse input");
        }

        char c = value[pos];
        if (c == '{') {
            return ParseObject(value, pos);
        }
        if (c == '[') {
            return ParseArray(value, pos);
        }
        if (c == '"') {
            return json(ParseString(value, pos));
        }
        if (value.compare(pos, 4, "true") == 0) {
            pos += 4;
            return json(true);
        }
        if (value.compare(pos, 5, "false") == 0) {
            pos += 5;
            return json(false);
        }
        if (value.compare(pos, 4, "null") == 0) {
            pos += 4;
            return json();
        }
        return ParseNumber(value, pos);
    }

    static json ParseObject(const std::string& value, size_t& pos) {
        json::object_t obj;
        ++pos;  // consume '{'
        SkipWhitespace(value, pos);
        if (pos < value.size() && value[pos] == '}') {
            ++pos;
            return json(obj);
        }
        while (pos < value.size()) {
            SkipWhitespace(value, pos);
            if (value[pos] != '"') {
                throw std::runtime_error("json stub parser failed to parse input");
            }
            std::string key = ParseString(value, pos);
            SkipWhitespace(value, pos);
            if (pos >= value.size() || value[pos] != ':') {
                throw std::runtime_error("json stub parser failed to parse input");
            }
            ++pos;
            json val = ParseValue(value, pos);
            obj.emplace(std::move(key), std::move(val));
            SkipWhitespace(value, pos);
            if (pos >= value.size()) {
                break;
            }
            if (value[pos] == ',') {
                ++pos;
                continue;
            }
            if (value[pos] == '}') {
                ++pos;
                break;
            }
            throw std::runtime_error("json stub parser failed to parse input");
        }
        return json(obj);
    }

    static json ParseArray(const std::string& value, size_t& pos) {
        json::array_t array;
        ++pos;  // consume '['
        SkipWhitespace(value, pos);
        if (pos < value.size() && value[pos] == ']') {
            ++pos;
            return json(array);
        }
        while (pos < value.size()) {
            json val = ParseValue(value, pos);
            array.push_back(std::move(val));
            SkipWhitespace(value, pos);
            if (pos >= value.size()) {
                break;
            }
            if (value[pos] == ',') {
                ++pos;
                continue;
            }
            if (value[pos] == ']') {
                ++pos;
                break;
            }
            throw std::runtime_error("json stub parser failed to parse input");
        }
        return json(array);
    }

    static std::string ParseString(const std::string& value, size_t& pos) {
        std::string result;
        if (value[pos] != '"') {
            throw std::runtime_error("json stub parser failed to parse input");
        }
        ++pos;
        while (pos < value.size()) {
            char c = value[pos++];
            if (c == '"') {
                break;
            }
            if (c == '\\' && pos < value.size()) {
                char escaped = value[pos++];
                switch (escaped) {
                    case '"':
                    case '\\':
                    case '/':
                        result.push_back(escaped);
                        break;
                    case 'b':
                        result.push_back('\b');
                        break;
                    case 'f':
                        result.push_back('\f');
                        break;
                    case 'n':
                        result.push_back('\n');
                        break;
                    case 'r':
                        result.push_back('\r');
                        break;
                    case 't':
                        result.push_back('\t');
                        break;
                    default:
                        result.push_back(escaped);
                        break;
                }
            } else {
                result.push_back(c);
            }
        }
        return result;
    }

    static json ParseNumber(const std::string& value, size_t& pos) {
        size_t start = pos;
        while (pos < value.size() && (std::isdigit(static_cast<unsigned char>(value[pos])) ||
                                      value[pos] == '-' || value[pos] == '+' ||
                                      value[pos] == '.' || value[pos] == 'e' || value[pos] == 'E')) {
            ++pos;
        }
        std::string number_str = value.substr(start, pos - start);
        double number_value = 0.0;
        if (!TryParseNumber(number_str, &number_value)) {
            throw std::runtime_error("json stub parser failed to parse input");
        }
        return json(number_value);
    }

    Type type_;
    bool boolean_;
    double number_;
    std::string string_;
    array_t array_;
    object_t object_;
};

}  // namespace nlohmann

#endif
