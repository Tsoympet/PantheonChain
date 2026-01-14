// ParthenonChain Full Node Daemon
// Copyright (c) 2024 ParthenonChain Developers
// Distributed under the MIT software license

#include <atomic>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
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
    std::thread network_thread_;
    std::thread validation_thread_;
    std::thread rpc_thread_;

    void NetworkLoop() {
        std::cout << "[Network] Thread started on port " << config_.network_port << std::endl;
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout << "[Network] Thread stopped" << std::endl;
    }

    void ValidationLoop() {
        std::cout << "[Validation] Thread started" << std::endl;
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout << "[Validation] Thread stopped" << std::endl;
    }

    void RPCLoop() {
        if (!config_.rpc_enabled)
            return;
        std::cout << "[RPC] Server started on port " << config_.rpc_port << std::endl;
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout << "[RPC] Server stopped" << std::endl;
    }

  public:
    explicit Node(const Config& config) : config_(config) {}

    bool Start() {
        std::cout << "=== ParthenonChain Node Starting ===" << std::endl;
        std::cout << "Data directory: " << config_.data_dir << std::endl;
        std::cout << "Network port: " << config_.network_port << std::endl;
        std::cout << "RPC enabled: " << (config_.rpc_enabled ? "yes" : "no") << std::endl;
        if (config_.rpc_enabled) {
            std::cout << "RPC port: " << config_.rpc_port << std::endl;
        }
        std::cout << "Mining: " << (config_.mining_enabled ? "enabled" : "disabled") << std::endl;

        running_ = true;

        network_thread_ = std::thread(&Node::NetworkLoop, this);
        validation_thread_ = std::thread(&Node::ValidationLoop, this);
        if (config_.rpc_enabled) {
            rpc_thread_ = std::thread(&Node::RPCLoop, this);
        }

        std::cout << "=== Node Started Successfully ===" << std::endl;
        return true;
    }

    void Stop() {
        std::cout << "\n=== Shutting Down Node ===" << std::endl;
        running_ = false;

        if (network_thread_.joinable())
            network_thread_.join();
        if (validation_thread_.joinable())
            validation_thread_.join();
        if (rpc_thread_.joinable())
            rpc_thread_.join();

        std::cout << "=== Node Stopped ===" << std::endl;
    }

    void WaitForShutdown() {
        while (running_) {
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
