#ifndef LEVELDB_WRITE_BATCH_STUB_H
#define LEVELDB_WRITE_BATCH_STUB_H

#include <string>
#include <utility>
#include <vector>

namespace leveldb {

class WriteBatch {
  public:
    struct Operation {
        enum class Type { kPut, kDelete };
        Type type;
        std::string key;
        std::string value;
    };

    void Put(const std::string& key, const std::string& value) {
        operations_.push_back(Operation{Operation::Type::kPut, key, value});
    }

    void Delete(const std::string& key) {
        operations_.push_back(Operation{Operation::Type::kDelete, key, std::string()});
    }

    const std::vector<Operation>& Operations() const { return operations_; }

  private:
    std::vector<Operation> operations_;
};

}  // namespace leveldb

#endif
