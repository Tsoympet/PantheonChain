// ParthenonChain Full Node Daemon
// Copyright (c) 2024 ParthenonChain Developers
// Distributed under the MIT software license

#include "node/chainparams.h"
#include "node/node.h"
#include "rpc/rpc_server.h"
#include "wallet/wallet.h"

#include <openssl/rand.h>

#include <array>
#include <cctype>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace parthenon {

struct Config {
    int network_port = 8333;
    int max_connections = 125;
    int network_timeout = 60;
    bool rpc_enabled = true;
    int rpc_port = 8332;
    std::string rpc_user = "";
    std::string rpc_password = "";
    bool rpc_allow_unauthenticated = false;
    std::string data_dir = "./data";
    std::string log_level = "info";
    bool mining_enabled = false;
    std::string network = "mainnet";
    bool network_port_configured = false;
    bool rpc_port_configured = false;
};

class ConfigParser {
  private:
    static bool TryParseInt(const std::string& value, int& out) {
        try {
            size_t parsed_chars = 0;
            const int parsed_value = std::stoi(value, &parsed_chars);
            if (parsed_chars != value.size()) {
                return false;
            }
            out = parsed_value;
            return true;
        } catch (...) {
            return false;
        }
    }

    static bool TryParseBool(const std::string& value, bool& out) {
        std::string normalized = value;
        for (auto& ch : normalized) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }

        if (normalized == "1" || normalized == "true" || normalized == "yes" ||
            normalized == "on") {
            out = true;
            return true;
        }
        if (normalized == "0" || normalized == "false" || normalized == "no" ||
            normalized == "off") {
            out = false;
            return true;
        }
        return false;
    }

    static std::string SanitizeScalarValue(const std::string& raw_value) {
        std::string value = raw_value;
        const auto comment_pos = value.find('#');
        if (comment_pos != std::string::npos) {
            value = value.substr(0, comment_pos);
        }

        const auto first_non_space = value.find_first_not_of(" \t\n\r");
        if (first_non_space == std::string::npos) {
            return "";
        }
        const auto last_non_space = value.find_last_not_of(" \t\n\r");
        return value.substr(first_non_space, last_non_space - first_non_space + 1);
    }

    static bool TryParsePort(const std::string& value, int& out) {
        int parsed = 0;
        if (!TryParseInt(value, parsed)) {
            return false;
        }
        if (parsed <= 0 || parsed > 65535) {
            return false;
        }
        out = parsed;
        return true;
    }

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

            const std::string scalar_value = SanitizeScalarValue(value);

            if (key == "network.port") {
                int parsed_port = 0;
                if (!TryParsePort(scalar_value, parsed_port)) {
                    std::cerr << "Warning: Invalid network.port '" << scalar_value
                if (!TryParsePort(value, parsed_port)) {
                    std::cerr << "Warning: Invalid network.port '" << value
                              << "'; keeping default/network-derived value" << std::endl;
                } else {
                    config.network_port = parsed_port;
                    config.network_port_configured = true;
                }
            }
            else if (key == "network.max_connections") {
                int parsed_connections = 0;
                if (!TryParseInt(scalar_value, parsed_connections) || parsed_connections <= 0) {
                    std::cerr << "Warning: Invalid network.max_connections '" << scalar_value
                if (!TryParseInt(value, parsed_connections) || parsed_connections <= 0) {
                    std::cerr << "Warning: Invalid network.max_connections '" << value
                              << "'; keeping default" << std::endl;
                } else {
                    config.max_connections = parsed_connections;
                }
            }
            else if (key == "network.timeout") {
                int parsed_timeout = 0;
                if (!TryParseInt(scalar_value, parsed_timeout) || parsed_timeout <= 0) {
                    std::cerr << "Warning: Invalid network.timeout '" << scalar_value
                if (!TryParseInt(value, parsed_timeout) || parsed_timeout <= 0) {
                    std::cerr << "Warning: Invalid network.timeout '" << value
                              << "'; keeping default" << std::endl;
                } else {
                    config.network_timeout = parsed_timeout;
                }
            }
            else if (key == "network.mode")
                config.network = scalar_value;
            else if (key == "rpc.enabled") {
                bool parsed_enabled = config.rpc_enabled;
                if (!TryParseBool(scalar_value, parsed_enabled)) {
                    std::cerr << "Warning: Invalid rpc.enabled '" << scalar_value
                config.network = value;
            else if (key == "rpc.enabled") {
                bool parsed_enabled = config.rpc_enabled;
                if (!TryParseBool(value, parsed_enabled)) {
                    std::cerr << "Warning: Invalid rpc.enabled '" << value
                              << "'; keeping default" << std::endl;
                } else {
                    config.rpc_enabled = parsed_enabled;
                }
            }
            else if (key == "rpc.port") {
                int parsed_port = 0;
                if (!TryParsePort(scalar_value, parsed_port)) {
                    std::cerr << "Warning: Invalid rpc.port '" << scalar_value
                if (!TryParsePort(value, parsed_port)) {
                    std::cerr << "Warning: Invalid rpc.port '" << value
                              << "'; keeping default/network-derived value" << std::endl;
                } else {
                    config.rpc_port = parsed_port;
                    config.rpc_port_configured = true;
                }
            }
            else if (key == "rpc.user")
                config.rpc_user = value;
            else if (key == "rpc.password")
                config.rpc_password = value;
            else if (key == "rpc.allow_unauthenticated") {
                bool parsed_allow_unauthenticated = config.rpc_allow_unauthenticated;
                if (!TryParseBool(scalar_value, parsed_allow_unauthenticated)) {
                    std::cerr << "Warning: Invalid rpc.allow_unauthenticated '" << scalar_value
                if (!TryParseBool(value, parsed_allow_unauthenticated)) {
                    std::cerr << "Warning: Invalid rpc.allow_unauthenticated '" << value
                              << "'; keeping default" << std::endl;
                } else {
                    config.rpc_allow_unauthenticated = parsed_allow_unauthenticated;
                }
            }
            else if (key == "data_dir")
                config.data_dir = value;
            else if (key == "log_level")
                config.log_level = value;
            else if (key == "mining.enabled") {
                bool parsed_mining_enabled = config.mining_enabled;
                if (!TryParseBool(scalar_value, parsed_mining_enabled)) {
                    std::cerr << "Warning: Invalid mining.enabled '" << scalar_value
                if (!TryParseBool(value, parsed_mining_enabled)) {
                    std::cerr << "Warning: Invalid mining.enabled '" << value
                              << "'; keeping default" << std::endl;
                } else {
                    config.mining_enabled = parsed_mining_enabled;
                }
            }
        }


        // Normalize and validate network mode
        for (auto& ch : config.network) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }

        auto mode_opt = node::ParseNetworkMode(config.network);
        if (!mode_opt) {
            std::cerr << "Warning: Unknown network.mode '" << config.network
                      << "', defaulting to mainnet" << std::endl;
            config.network = "mainnet";
            mode_opt = node::NetworkMode::MAINNET;
        }

        config.network = node::NetworkModeToString(*mode_opt);
        const auto params = node::GetNetworkParams(*mode_opt);
        if (!config.network_port_configured) {
            config.network_port = static_cast<int>(params.default_p2p_port);
        }
        if (!config.rpc_port_configured) {
            config.rpc_port = static_cast<int>(params.default_rpc_port);
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

    static std::optional<std::array<uint8_t, 32>> LoadOrGenerateWalletSeed(
        const std::filesystem::path& data_dir, std::string& error_message) {
        const auto seed_path = data_dir / "wallet.seed";
        std::array<uint8_t, 32> seed{};

        if (std::filesystem::exists(seed_path)) {
            std::ifstream seed_file(seed_path, std::ios::binary);
            if (!seed_file) {
                error_message = "Failed to load wallet seed from " + seed_path.string() + ": " +
                                std::strerror(errno);
                return std::nullopt;
            }
            seed_file.read(reinterpret_cast<char*>(seed.data()),
                           static_cast<std::streamsize>(seed.size()));
            if (seed_file.gcount() != static_cast<std::streamsize>(seed.size())) {
                error_message = "Wallet seed at " + seed_path.string() +
                                " is invalid (expected 32 bytes)";
                return std::nullopt;
            }
            return seed;
        }

        if (RAND_status() != 1) {
            if (RAND_poll() != 1) {
                error_message = "OpenSSL random number generator not seeded; check system entropy";
                return std::nullopt;
            }
        }

        if (RAND_bytes(seed.data(), static_cast<int>(seed.size())) != 1) {
            error_message =
                "Failed to generate wallet seed; OpenSSL RAND_bytes failed (check installation)";
            return std::nullopt;
        }
#ifndef _WIN32
        const int seed_fd =
            ::open(seed_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (seed_fd == -1) {
            error_message = "Failed to create wallet seed at " + seed_path.string() + ": " +
                            std::strerror(errno);
            return std::nullopt;
        }
        const auto bytes_written = ::write(seed_fd, seed.data(), seed.size());
        if (bytes_written != static_cast<ssize_t>(seed.size())) {
            error_message = "Failed to save wallet seed at " + seed_path.string() +
                            "; check disk space and permissions";
            ::close(seed_fd);
            return std::nullopt;
        }
        ::close(seed_fd);
#else
        std::ofstream seed_file(seed_path, std::ios::binary | std::ios::trunc);
        if (!seed_file) {
            error_message = "Failed to create wallet seed at " + seed_path.string() + ": " +
                            std::strerror(errno);
            return std::nullopt;
        }

        std::error_code permissions_error;
        std::filesystem::permissions(seed_path,
                                     std::filesystem::perms::owner_read |
                                         std::filesystem::perms::owner_write,
                                     std::filesystem::perm_options::replace, permissions_error);
        if (permissions_error) {
            std::cerr << "Warning: Failed to set wallet.seed permissions: "
                      << permissions_error.message() << std::endl;
        }

        seed_file.write(reinterpret_cast<const char*>(seed.data()),
                        static_cast<std::streamsize>(seed.size()));
        if (!seed_file) {
            error_message = "Failed to save wallet seed at " + seed_path.string() +
                            "; check disk space and permissions";
            return std::nullopt;
        }
#endif
        return seed;
    }

  public:
    explicit Node(const Config& config) : config_(config) {}

    bool Start() {
        if (running_.load()) {
            std::cerr << "Node already running" << std::endl;
            return false;
        }

        std::cout << "=== ParthenonChain Node Starting ===" << std::endl;
        std::cout << "Data directory: " << config_.data_dir << std::endl;
        std::cout << "Network mode: " << config_.network << std::endl;
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

        const auto parsed_mode = node::ParseNetworkMode(config_.network);
        if (!parsed_mode.has_value()) {
            std::cerr << "Invalid internal network mode '" << config_.network
                      << "' after config parsing" << std::endl;
            return false;
        }
        const auto network_mode = *parsed_mode;
        }
        const auto network_mode = *parsed_mode;
        }
        const auto network_mode = *parsed_mode;
        }
        const auto network_mode = *parsed_mode;
        }
        const auto network_mode = *parsed_mode;
        }
        const auto network_mode = *parsed_mode;
        }
        const auto network_mode = *parsed_mode;
        }
        const auto network_mode = *parsed_mode;
        }
        const auto network_mode = *parsed_mode;
        }
        const auto network_mode = *parsed_mode;
        }
        const auto network_mode = *parsed_mode;
        }
        const auto network_mode = *parsed_mode;
        }
        const auto network_mode = *parsed_mode;
        }
        const auto network_mode = *parsed_mode;
        const auto network_mode = parsed_mode.value_or(node::NetworkMode::MAINNET);
        if (!parsed_mode.has_value()) {
            std::cerr << "Unknown network mode '" << config_.network
                      << "', defaulting to mainnet (supported: mainnet/testnet/regtest)"
                      << std::endl;
        }
        std::cout << "Selected network: " << node::NetworkModeToString(network_mode)
                  << std::endl;

        core_node_ =
            std::make_unique<node::Node>(config_.data_dir, config_.network_port, network_mode);

        std::string seed_error;
        auto seed = LoadOrGenerateWalletSeed(config_.data_dir, seed_error);
        if (!seed) {
            std::cerr << seed_error << std::endl;
            return false;
        }
        wallet_ = std::make_shared<wallet::Wallet>(*seed);

        if (!core_node_->Start()) {
            std::cerr << "Failed to start core node" << std::endl;
            return false;
        }

        core_node_->AttachWallet(wallet_);

        if (config_.rpc_enabled) {
            const bool has_rpc_user = !config_.rpc_user.empty();
            const bool has_rpc_password = !config_.rpc_password.empty();
            if (!config_.rpc_allow_unauthenticated && (!has_rpc_user || !has_rpc_password)) {
                std::cerr << "Refusing to start RPC server without credentials. "
                          << "Set rpc.user and rpc.password, or set "
                          << "rpc.allow_unauthenticated=true for local development only."
                          << std::endl;
                core_node_->Stop();
                return false;
            }

            if (config_.rpc_allow_unauthenticated && (!has_rpc_user || !has_rpc_password)) {
                std::cerr << "Warning: RPC authentication disabled via "
                          << "rpc.allow_unauthenticated=true; use only in trusted environments."
                          << std::endl;
            }

            rpc_server_ = std::make_unique<rpc::RPCServer>(config_.rpc_port);
            rpc_server_->SetNode(core_node_.get());
            rpc_server_->SetWallet(wallet_.get());
            rpc_server_->ConfigureBasicAuth(config_.rpc_user, config_.rpc_password);
            if (!rpc_server_->Start()) {
                std::cerr << "Failed to start RPC server" << std::endl;
                core_node_->Stop();
                return false;
            }
        }

        if (config_.mining_enabled) {
            auto mining_address = wallet_->GenerateAddress("mining");
            if (mining_address.pubkey.empty()) {
                std::cerr << "Failed to generate mining address" << std::endl;
                if (rpc_server_ && rpc_server_->IsRunning()) {
                    rpc_server_->Stop();
                }
                if (core_node_) {
                    core_node_->Stop();
                }
                return false;
            }
            core_node_->StartMining(mining_address.pubkey);
        }

        running_ = true;
        std::cout << "=== Node Started Successfully ===" << std::endl;
        return true;
    }

    void Stop() {
        const bool node_running = core_node_ && core_node_->IsRunning();
        const bool rpc_running = rpc_server_ && rpc_server_->IsRunning();
        if (!running_.load() && !node_running && !rpc_running) {
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
