// ParthenonChain Full Node Daemon
// Copyright (c) 2024 ParthenonChain Developers
// Distributed under the MIT software license

#include "node/node.h"
#include "rpc/rpc_server.h"
#include "wallet/wallet.h"

#include <array>
#include <atomic>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>

namespace parthenon {

struct Config {
    int network_port = 8333;
    int max_connections = 125;
    int network_timeout = 60;
    bool rpc_enabled = true;
    int rpc_port = 8332;
    std::string rpc_user = "";
    std::string rpc_password = "";
    std::string data_dir = "./data";
    std::string log_level = "info";
    bool mining_enabled = false;
};

class ConfigParser {
  public:
    static Config Parse(const std::string& filepath) {
        Config config;
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open config file " << filepath << ", using defaults"
                      << std::endl;
            return config;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#')
                continue;

            size_t eq = line.find('=');
            if (eq == std::string::npos)
                continue;

            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (key == "network.port")
                config.network_port = std::stoi(value);
            else if (key == "network.max_connections")
                config.max_connections = std::stoi(value);
            else if (key == "network.timeout")
                config.network_timeout = std::stoi(value);
            else if (key == "rpc.enabled")
                config.rpc_enabled = (value == "true" || value == "1");
            else if (key == "rpc.port")
                config.rpc_port = std::stoi(value);
            else if (key == "rpc.user")
                config.rpc_user = value;
            else if (key == "rpc.password")
                config.rpc_password = value;
            else if (key == "data_dir")
                config.data_dir = value;
            else if (key == "log_level")
                config.log_level = value;
            else if (key == "mining.enabled")
                config.mining_enabled = (value == "true" || value == "1");
        }

        return config;
    }
};

class Node {
  private:
    Config config_;
    std::atomic<bool> running_{false};
    std::unique_ptr<node::Node> core_node_;
    std::shared_ptr<wallet::Wallet> wallet_;
    std::unique_ptr<rpc::RPCServer> rpc_server_;

    static std::array<uint8_t, 32> GenerateWalletSeed() {
        std::array<uint8_t, 32> seed{};
        std::random_device rd;
        std::uniform_int_distribution<int> dist(0, 255);
        for (auto& byte : seed) {
            byte = static_cast<uint8_t>(dist(rd));
        }
        return seed;
    }

  public:
    explicit Node(const Config& config) : config_(config) {}

    bool Start() {
        if (running_.load()) {
            return false;
        }

        std::cout << "=== ParthenonChain Node Starting ===" << std::endl;
        std::cout << "Data directory: " << config_.data_dir << std::endl;
        std::cout << "Network port: " << config_.network_port << std::endl;
        std::cout << "RPC enabled: " << (config_.rpc_enabled ? "yes" : "no") << std::endl;
        if (config_.rpc_enabled) {
            std::cout << "RPC port: " << config_.rpc_port << std::endl;
        }
        std::cout << "Mining: " << (config_.mining_enabled ? "enabled" : "disabled") << std::endl;

        try {
            std::filesystem::create_directories(config_.data_dir);
        } catch (const std::exception& e) {
            std::cerr << "Failed to prepare data directory: " << e.what() << std::endl;
            return false;
        }

        core_node_ = std::make_unique<node::Node>(config_.data_dir, config_.network_port);
        wallet_ = std::make_shared<wallet::Wallet>(GenerateWalletSeed());

        if (!core_node_->Start()) {
            std::cerr << "Failed to start core node" << std::endl;
            return false;
        }

        core_node_->AttachWallet(wallet_);

        if (config_.rpc_enabled) {
            rpc_server_ = std::make_unique<rpc::RPCServer>(config_.rpc_port);
            rpc_server_->SetNode(core_node_.get());
            rpc_server_->SetWallet(wallet_.get());
            if (!rpc_server_->Start()) {
                std::cerr << "Failed to start RPC server" << std::endl;
                core_node_->Stop();
                return false;
            }
        }

        if (config_.mining_enabled) {
            auto mining_address = wallet_->GenerateAddress("mining");
            core_node_->StartMining(mining_address.pubkey);
        }

        running_ = true;
        std::cout << "=== Node Started Successfully ===" << std::endl;
        return true;
    }

    void Stop() {
        if (!running_.load() && (!core_node_ || !core_node_->IsRunning())) {
            return;
        }
        std::cout << "\n=== Shutting Down Node ===" << std::endl;
        running_ = false;

        if (rpc_server_ && rpc_server_->IsRunning()) {
            rpc_server_->Stop();
        }
        if (core_node_) {
            core_node_->Stop();
        }

        std::cout << "=== Node Stopped ===" << std::endl;
    }

    void WaitForShutdown() {
        while (running_) {
            if (core_node_ && !core_node_->IsRunning()) {
                running_ = false;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

static Node* g_node = nullptr;

void SignalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << std::endl;
    if (g_node) {
        g_node->Stop();
    }
}

}  // namespace parthenon

int main(int argc, char* argv[]) {
    std::cout << "ParthenonChain Node Daemon v1.0.0" << std::endl;
    std::cout << "Copyright (c) 2024 ParthenonChain Developers" << std::endl;
    std::cout << std::endl;

    std::string config_file = "parthenond.conf";
    if (argc > 1) {
        config_file = argv[1];
    }

    // Parse configuration
    parthenon::Config config = parthenon::ConfigParser::Parse(config_file);

    // Create node
    parthenon::Node node(config);
    parthenon::g_node = &node;

    // Setup signal handlers
    std::signal(SIGINT, parthenon::SignalHandler);
    std::signal(SIGTERM, parthenon::SignalHandler);

    // Start node
    if (!node.Start()) {
        std::cerr << "Failed to start node" << std::endl;
        return 1;
    }

    // Wait for shutdown signal
    node.WaitForShutdown();

    return 0;
}
