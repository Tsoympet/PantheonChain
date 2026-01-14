// ParthenonChain - Contract Indexer Implementation
// Indexes EVM contract deployments and events

#include "contract_indexer.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace parthenon {
namespace layer2 {
namespace indexers {

class ContractIndexer::Impl {
  public:
    Impl() : is_open_(false) {}

    bool Open(const std::string& db_path) {
        db_path_ = db_path;

        // Load existing index
        std::ifstream file(db_path_ + "/contract_index.dat", std::ios::binary);
        if (file.is_open()) {
            // Load contract index from file
            file.close();
        }

        is_open_ = true;
        return true;
    }

    void Close() {
        if (!is_open_)
            return;

        // Save index to file
        std::ofstream file(db_path_ + "/contract_index.dat", std::ios::binary | std::ios::trunc);
        if (file.is_open()) {
            // Save contract index
            file.close();
        }

        is_open_ = false;
    }

    void IndexContractDeployment(const std::array<uint8_t, 20>& address,
                                 const std::vector<uint8_t>& code, uint32_t height) {
        ContractInfo info;
        info.address = address;
        info.code = code;
        info.deployment_height = height;
        info.event_count = 0;

        contracts_[address] = info;
    }

    void IndexEvent(const ContractEvent& event) {
        events_.push_back(event);

        // Update contract event count
        auto it = contracts_.find(event.contract_address);
        if (it != contracts_.end()) {
            it->second.event_count++;
        }

        // Index by contract
        events_by_contract_[event.contract_address].push_back(event);

        // Index by topic (for efficient filtering)
        for (const auto& topic : event.topics) {
            events_by_topic_[topic].push_back(events_.size() - 1);
        }
    }

    std::vector<ContractEvent> GetEventsByContract(const std::array<uint8_t, 20>& contract_address,
                                                   uint32_t limit) {
        std::vector<ContractEvent> result;

        auto it = events_by_contract_.find(contract_address);
        if (it != events_by_contract_.end()) {
            size_t count = std::min(static_cast<size_t>(limit), it->second.size());
            result.assign(it->second.begin(), it->second.begin() + count);
        }

        return result;
    }

    std::vector<ContractEvent> GetEventsByTopic(const std::array<uint8_t, 32>& topic,
                                                uint32_t limit) {
        std::vector<ContractEvent> result;

        auto it = events_by_topic_.find(topic);
        if (it != events_by_topic_.end()) {
            for (size_t i = 0; i < std::min(static_cast<size_t>(limit), it->second.size()); ++i) {
                size_t event_idx = it->second[i];
                if (event_idx < events_.size()) {
                    result.push_back(events_[event_idx]);
                }
            }
        }

        return result;
    }

    std::optional<ContractIndexer::ContractInfo>
    GetContractInfo(const std::array<uint8_t, 20>& address) {
        auto it = contracts_.find(address);
        if (it != contracts_.end()) {
            // Convert internal ContractInfo to external ContractInfo
            ContractIndexer::ContractInfo result;
            result.address = it->second.address;
            result.code = it->second.code;
            result.deployment_height = it->second.deployment_height;
            result.event_count = it->second.event_count;
            return result;
        }
        return std::nullopt;
    }

    size_t GetContractCount() const { return contracts_.size(); }

    size_t GetEventCount() const { return events_.size(); }

    std::vector<std::array<uint8_t, 20>> GetAllContracts() const {
        std::vector<std::array<uint8_t, 20>> result;
        for (const auto& pair : contracts_) {
            result.push_back(pair.first);
        }
        return result;
    }

  private:
    struct ContractInfo {
        std::array<uint8_t, 20> address;
        std::vector<uint8_t> code;
        uint32_t deployment_height;
        uint64_t event_count;
    };

    std::string db_path_;
    bool is_open_;
    std::map<std::array<uint8_t, 20>, ContractInfo> contracts_;
    std::vector<ContractEvent> events_;
    std::map<std::array<uint8_t, 20>, std::vector<ContractEvent>> events_by_contract_;
    std::map<std::array<uint8_t, 32>, std::vector<size_t>> events_by_topic_;
};

ContractIndexer::ContractIndexer() : impl_(std::make_unique<Impl>()) {}

ContractIndexer::~ContractIndexer() {
    if (impl_) {
        impl_->Close();
    }
}

bool ContractIndexer::Open(const std::string& db_path) {
    return impl_->Open(db_path);
}

void ContractIndexer::Close() {
    impl_->Close();
}

void ContractIndexer::IndexContractDeployment(const std::array<uint8_t, 20>& address,
                                              const std::vector<uint8_t>& code, uint32_t height) {
    impl_->IndexContractDeployment(address, code, height);
}

void ContractIndexer::IndexEvent(const ContractEvent& event) {
    impl_->IndexEvent(event);
}

std::vector<ContractEvent>
ContractIndexer::GetEventsByContract(const std::array<uint8_t, 20>& contract_address,
                                     uint32_t limit) {
    return impl_->GetEventsByContract(contract_address, limit);
}

std::vector<ContractEvent> ContractIndexer::GetEventsByTopic(const std::array<uint8_t, 32>& topic,
                                                             uint32_t limit) {
    return impl_->GetEventsByTopic(topic, limit);
}

std::optional<ContractIndexer::ContractInfo>
ContractIndexer::GetContractInfo(const std::array<uint8_t, 20>& address) {
    auto info_opt = impl_->GetContractInfo(address);
    if (info_opt) {
        ContractInfo result;
        result.address = info_opt->address;
        result.code = info_opt->code;
        result.deployment_height = info_opt->deployment_height;
        result.event_count = info_opt->event_count;
        return result;
    }
    return std::nullopt;
}

size_t ContractIndexer::GetContractCount() const {
    return impl_->GetContractCount();
}

size_t ContractIndexer::GetEventCount() const {
    return impl_->GetEventCount();
}

std::vector<std::array<uint8_t, 20>> ContractIndexer::GetAllContracts() const {
    return impl_->GetAllContracts();
}

}  // namespace indexers
}  // namespace layer2
}  // namespace parthenon
