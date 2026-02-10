#ifndef LEVELDB_DB_STUB_H
#define LEVELDB_DB_STUB_H

#include <map>
#include <string>
#include <utility>

#include "leveldb/write_batch.h"

namespace leveldb {

class Status {
  public:
    enum class Code { kOk, kNotFound, kInvalidArgument };

    Status() : code_(Code::kOk) {}
    static Status OK() { return Status(Code::kOk); }
    static Status NotFound() { return Status(Code::kNotFound); }
    static Status InvalidArgument() { return Status(Code::kInvalidArgument); }

    bool ok() const { return code_ == Code::kOk; }

  private:
    explicit Status(Code code) : code_(code) {}
    Code code_;
};

class Slice {
  public:
    Slice() = default;
    explicit Slice(std::string value) : value_(std::move(value)) {}

    const std::string& ToString() const { return value_; }

  private:
    std::string value_;
};

struct Options {
    bool create_if_missing = false;
};

struct ReadOptions {};
struct WriteOptions {};

class Iterator {
  public:
    virtual ~Iterator() = default;
    virtual void SeekToFirst() = 0;
    virtual bool Valid() const = 0;
    virtual void Next() = 0;
    virtual Slice key() const = 0;
    virtual Slice value() const = 0;
    virtual Status status() const = 0;
};

class DB {
  private:
    class MapIterator final : public Iterator {
      public:
        explicit MapIterator(const std::map<std::string, std::string>* data)
            : data_(data), iter_(), started_(false) {
            if (data_ != nullptr) {
                iter_ = data_->end();
            }
        }

        void SeekToFirst() override {
            if (data_ == nullptr) {
                return;
            }
            iter_ = data_->begin();
            started_ = true;
        }

        bool Valid() const override {
            return data_ != nullptr && started_ && iter_ != data_->end();
        }

        void Next() override {
            if (data_ == nullptr) {
                return;
            }
            if (!started_) {
                SeekToFirst();
                return;
            }
            if (iter_ != data_->end()) {
                ++iter_;
            }
        }

        Slice key() const override {
            if (!Valid()) {
                return Slice();
            }
            return Slice(iter_->first);
        }

        Slice value() const override {
            if (!Valid()) {
                return Slice();
            }
            return Slice(iter_->second);
        }

        Status status() const override { return Status::OK(); }

      private:
        const std::map<std::string, std::string>* data_;
        std::map<std::string, std::string>::const_iterator iter_;
        bool started_;
    };

  public:
    static Status Open(const Options& options, const std::string& name, DB** dbptr) {
        (void)name;
        if (dbptr == nullptr) {
            return Status::InvalidArgument();
        }
        if (!options.create_if_missing) {
            return Status::InvalidArgument();
        }
        *dbptr = new DB();
        return Status::OK();
    }

    Status Put(const WriteOptions& options, const std::string& key, const std::string& value) {
        (void)options;
        data_[key] = value;
        return Status::OK();
    }

    Status Delete(const WriteOptions& options, const std::string& key) {
        (void)options;
        auto it = data_.find(key);
        if (it == data_.end()) {
            return Status::NotFound();
        }
        data_.erase(it);
        return Status::OK();
    }

    Status Get(const ReadOptions& options, const std::string& key, std::string* value) {
        (void)options;
        auto it = data_.find(key);
        if (it == data_.end()) {
            return Status::NotFound();
        }
        if (value != nullptr) {
            *value = it->second;
        }
        return Status::OK();
    }

    Status Write(const WriteOptions& options, WriteBatch* batch) {
        (void)options;
        if (batch == nullptr) {
            return Status::InvalidArgument();
        }
        for (const auto& op : batch->Operations()) {
            if (op.type == WriteBatch::Operation::Type::kPut) {
                data_[op.key] = op.value;
            } else {
                data_.erase(op.key);
            }
        }
        return Status::OK();
    }

    // Caller owns the returned iterator and is responsible for deleting it.
    Iterator* NewIterator(const ReadOptions& options) {
        (void)options;
        return new MapIterator(&data_);
    }

  private:
    std::map<std::string, std::string> data_;
};

}  // namespace leveldb

#endif
