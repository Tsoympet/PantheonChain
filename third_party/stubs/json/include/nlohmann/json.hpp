#ifndef NLOHMANN_JSON_STUB_HPP
#define NLOHMANN_JSON_STUB_HPP

#include <initializer_list>
#include <map>
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

    static json parse(const std::string& /*unused*/) { return json(); }

    static json array() { return json(array_t{}); }

    bool is_array() const { return type_ == Type::kArray; }
    bool is_object() const { return type_ == Type::kObject; }
    bool is_number() const { return type_ == Type::kNumber; }

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
                return "[]";
            case Type::kObject:
                return "{}";
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

    Type type_;
    bool boolean_;
    double number_;
    std::string string_;
    array_t array_;
    object_t object_;
};

}  // namespace nlohmann

#endif
