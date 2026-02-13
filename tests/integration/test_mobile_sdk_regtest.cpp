#include "mobile_sdk.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <arpa/inet.h>
#include <cassert>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <optional>
#include <sstream>
#include <string_view>
#include <sys/socket.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace parthenon::mobile;

namespace {

using json = nlohmann::json;

struct RpcState {
    uint64_t block_height{1};
    std::map<uint64_t, std::string> block_hashes;
    std::map<uint64_t, std::vector<std::string>> block_txs;
    std::map<std::string, uint64_t> balances{
        {"TALANTON", 1000},
        {"DRACHMA", 2500},
        {"OBOLOS", 500}};
    uint64_t next_tx{1};
};

class RpcServer {
  public:
    RpcServer() {
        state_.block_hashes[1] = "blockhash-1";
        state_.block_txs[1] = {"genesis-tx"};
    }

    void Start(uint16_t port) {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            throw std::runtime_error("Failed to create server socket");
        }

        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            close(server_fd_);
            server_fd_ = -1;
            throw std::runtime_error("Failed to bind server socket");
        }

        if (listen(server_fd_, 4) != 0) {
            close(server_fd_);
            server_fd_ = -1;
            throw std::runtime_error("Failed to listen on server socket");
        }

        running_.store(true);
        thread_ = std::thread([this]() { Run(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void Stop() {
        running_.store(false);
        if (server_fd_ >= 0) {
            shutdown(server_fd_, SHUT_RDWR);
            close(server_fd_);
            server_fd_ = -1;
        }
        if (thread_.joinable()) {
            thread_.join();
        }
    }

  private:
    void Run() {
        while (running_.load()) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
            if (client_fd < 0) {
                if (!running_.load()) {
                    break;
                }
                continue;
            }
            HandleClient(client_fd);
            close(client_fd);
        }
    }

    void HandleClient(int client_fd) {
        std::string request;
        size_t header_end = std::string::npos;
        size_t content_length = 0;
        char buffer[4096];
        while (true) {
            ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes <= 0) {
                break;
            }
            request.append(buffer, static_cast<size_t>(bytes));
            if (header_end == std::string::npos) {
                header_end = request.find("\r\n\r\n");
                if (header_end != std::string::npos) {
                    content_length = ParseContentLength(request.substr(0, header_end));
                }
            }
            if (header_end != std::string::npos &&
                request.size() >= header_end + 4 + content_length) {
                break;
            }
        }

        if (header_end == std::string::npos) {
            return;
        }

        std::string body = request.substr(header_end + 4, content_length);
        std::string response_body = HandleRpc(body);

        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Content-Length: " << response_body.size() << "\r\n";
        response << "Connection: close\r\n\r\n";
        response << response_body;

        std::string response_data = response.str();
        size_t offset = 0;
        while (offset < response_data.size()) {
            ssize_t sent =
                send(client_fd, response_data.data() + offset, response_data.size() - offset, 0);
            if (sent <= 0) {
                break;
            }
            offset += static_cast<size_t>(sent);
        }
    }

    size_t ParseContentLength(const std::string& headers) const {
        std::istringstream stream(headers);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.rfind("Content-Length:", 0) == 0) {
                auto value = line.substr(std::string("Content-Length:").size());
                size_t start = value.find_first_not_of(' ');
                if (start != std::string::npos) {
                    value = value.substr(start);
                }
                try {
                    return static_cast<size_t>(std::stoul(value));
                } catch (...) {
                    return 0;
                }
            }
        }
        return 0;
    }

    std::string HandleRpc(const std::string& body) {
        json response;
        response["jsonrpc"] = "2.0";

        json request;
        try {
            request = json::parse(body);
        } catch (const std::exception&) {
            response["id"] = 0;
            response["error"] = {{"message", "Invalid request"}};
            return response.dump();
        }

        if (!request.is_object()) {
            response["id"] = 0;
            response["error"] = {{"message", "Invalid request"}};
            return response.dump();
        }

        response["id"] = request.value("id", 0);
        const auto method = request.value("method", "");
        json params = json::array();
        if (request.contains("params")) {
            params = request["params"];
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if (method == "getblockcount") {
            response["result"] = state_.block_height;
        } else if (method == "getblock") {
            if (!params.is_array() || params.empty() || !params[0].is_number_unsigned()) {
                response["error"] = {{"message", "Invalid params"}};
            } else {
                uint64_t height = params[0].get<uint64_t>();
                if (state_.block_hashes.find(height) == state_.block_hashes.end()) {
                    response["error"] = {{"message", "Block not found"}};
                } else {
                    json block;
                    block["hash"] = state_.block_hashes[height];
                    block["timestamp"] = 1700000000 + height;
                    json tx_array = json::array();
                    for (const auto& txid : state_.block_txs[height]) {
                        tx_array.push_back(txid);
                    }
                    block["tx"] = tx_array;
                    response["result"] = block;
                }
            }
        } else if (method == "getbalance") {
            if (!params.is_array() || params.empty() || !params[0].is_string()) {
                response["error"] = {{"message", "Invalid params"}};
            } else {
                const auto asset = params[0].get<std::string>();
                response["result"] = json{{"balance", state_.balances[asset]}};
            }
        } else if (method == "sendtoaddress") {
            if (!params.is_array() || params.size() < 3) {
                response["error"] = {{"message", "Invalid params"}};
            } else {
                std::string txid = "regtest-tx-" + std::to_string(state_.next_tx++);
                uint64_t height = ++state_.block_height;
                state_.block_hashes[height] = "blockhash-" + std::to_string(height);
                state_.block_txs[height] = {txid};
                response["result"] = txid;
            }
        } else if (method == "getinfo") {
            response["result"] = json{{"blocks", state_.block_height},
                                      {"connections", 3},
                                      {"syncing", false}};
        } else {
            response["error"] = {{"message", "Method not found"}};
        }

        return response.dump();
    }

    std::atomic<bool> running_{false};
    int server_fd_{-1};
    std::thread thread_;
    std::mutex mutex_;
    RpcState state_;
};

}  // namespace

int main() {
    RpcServer server;
    const uint16_t port = 18443;
    server.Start(port);

    NetworkConfig config;
    config.endpoint = "http://127.0.0.1:" + std::to_string(port) + "/";
    config.network_id = "regtest";
    config.chain_id = 18444;

    auto wallet = Wallet::Generate();
    assert(wallet);
    assert(!wallet->GetAddress().empty());
    assert(!wallet->GetPublicKey().empty());

    auto signature = wallet->SignMessage("regtest");
    assert(!signature.empty());

    MobileClient client(config);

    std::optional<Balance> balance;
    std::optional<std::string> balance_error;
    client.GetBalance("", [&](std::optional<Balance> result, std::optional<std::string> error) {
        balance = result;
        balance_error = error;
    });
    assert(balance.has_value());
    assert(!balance_error.has_value());
    assert(balance->taln == 1000);

    Transaction tx;
    tx.to = wallet->GetAddress();
    tx.amount = 42;
    tx.asset = "TALN";

    std::optional<std::string> txid;
    std::optional<std::string> tx_error;
    client.SendTransaction(tx, [&](std::optional<std::string> result, std::optional<std::string> error) {
        txid = result;
        tx_error = error;
    });
    assert(txid.has_value());
    assert(!tx_error.has_value());

    std::optional<std::vector<TransactionHistory>> history;
    std::optional<std::string> history_error;
    client.GetTransactionHistory("", 10,
                                 [&](std::vector<TransactionHistory> result,
                                     std::optional<std::string> error) {
                                     history = std::move(result);
                                     history_error = error;
                                 });
    assert(history.has_value());
    assert(!history_error.has_value());
    bool found = false;
    for (const auto& entry : *history) {
        if (entry.txid == *txid) {
            found = true;
            break;
        }
    }
    assert(found);

    std::optional<TransactionHistory> tx_info;
    std::optional<std::string> tx_info_error;
    client.GetTransaction(*txid,
                          [&](std::optional<TransactionHistory> result,
                              std::optional<std::string> error) {
                              tx_info = result;
                              tx_info_error = error;
                          });
    assert(tx_info.has_value());
    assert(!tx_info_error.has_value());
    assert(tx_info->txid == *txid);

    std::optional<MobileClient::NetworkStatus> status;
    std::optional<std::string> status_error;
    client.GetNetworkStatus([&](std::optional<MobileClient::NetworkStatus> result,
                                std::optional<std::string> error) {
        status = result;
        status_error = error;
    });
    assert(status.has_value());
    assert(!status_error.has_value());
    assert(status->block_height >= 2);

    server.Stop();

    std::cout << "✓ Mobile SDK regtest integration test passed\n";
    return 0;
}
