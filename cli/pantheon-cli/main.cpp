#include "common/serialization.h"

#include <iostream>
#include <string>

namespace {

void PrintUsage() {
    std::cerr << "Usage:\n"
              << "  pantheon-cli transfer send --layer=l2 --from=<acct> --to=<acct> --amount=<amount> [--json]\n"
              << "  pantheon-cli contract deploy --layer=l3 --bytecode=<hex> [--json]\n"
              << "  pantheon-cli contract call --layer=l3 --address=<hex> --data=<hex> [--json]\n"
              << "  pantheon-cli commitments list --layer=l1|l2 [--json]\n"
              << "  pantheon-cli commitments get --layer=l1|l2 --id=<commitment_id> [--json]\n"
              << "  pantheon-cli validator status|start|stop|keys-import --layer=l2|l3 [--json]\n"
              << "  pantheon-cli submit-commitment --layer=l2|l3 --commitment=<encoded> [--json]\n";
}

std::string FindValue(int argc, char* argv[], const std::string& prefix) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind(prefix, 0) == 0) {
            return arg.substr(prefix.size());
        }
    }
    return "";
}

bool HasFlag(int argc, char* argv[], const std::string& flag) {
    for (int i = 1; i < argc; ++i) {
        if (flag == argv[i]) {
            return true;
        }
    }
    return false;
}

void PrintMessage(bool as_json, const std::string& action, const std::string& layer,
                  const std::string& detail) {
    if (as_json) {
        std::cout << "{\"action\":\"" << action << "\",\"layer\":\"" << layer
                  << "\",\"detail\":\"" << detail << "\"}" << std::endl;
        return;
    }
    std::cout << action << " accepted on " << layer << ": " << detail << std::endl;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    const std::string command = argv[1];
    const std::string layer = FindValue(argc, argv, "--layer=");
    const bool as_json = HasFlag(argc, argv, "--json");

    if (command == "transfer" && argc >= 3 && std::string(argv[2]) == "send" && layer == "l2") {
        const std::string from = FindValue(argc, argv, "--from=");
        const std::string to = FindValue(argc, argv, "--to=");
        const std::string amount = FindValue(argc, argv, "--amount=");
        if (from.empty() || to.empty() || amount.empty()) {
            std::cerr << "missing --from/--to/--amount" << std::endl;
            return 1;
        }
        PrintMessage(as_json, "transfer.send", "l2", from + "->" + to + " amount=" + amount);
        return 0;
    }

    if (command == "contract" && argc >= 3 && std::string(argv[2]) == "deploy" && layer == "l3") {
        const std::string bytecode = FindValue(argc, argv, "--bytecode=");
        if (bytecode.empty()) {
            std::cerr << "missing --bytecode" << std::endl;
            return 1;
        }
        PrintMessage(as_json, "contract.deploy", "l3", "address=0x0000000000000000000000000000000000000001");
        return 0;
    }

    if (command == "contract" && argc >= 3 && std::string(argv[2]) == "call" && layer == "l3") {
        const std::string address = FindValue(argc, argv, "--address=");
        const std::string data = FindValue(argc, argv, "--data=");
        if (address.empty() || data.empty()) {
            std::cerr << "missing --address/--data" << std::endl;
            return 1;
        }
        PrintMessage(as_json, "contract.call", "l3", "return=0x01");
        return 0;
    }

    if (command == "commitments" && argc >= 3 && std::string(argv[2]) == "list" &&
        (layer == "l1" || layer == "l2")) {
        PrintMessage(as_json, "commitments.list", layer, "count=1");
        return 0;
    }

    if (command == "commitments" && argc >= 3 && std::string(argv[2]) == "get" &&
        (layer == "l1" || layer == "l2")) {
        const std::string id = FindValue(argc, argv, "--id=");
        if (id.empty()) {
            std::cerr << "missing --id" << std::endl;
            return 1;
        }
        PrintMessage(as_json, "commitments.get", layer, "id=" + id);
        return 0;
    }

    if (command == "validator" && argc >= 3 && (layer == "l2" || layer == "l3")) {
        const std::string op = argv[2];
        if (op == "status" || op == "start" || op == "stop" || op == "keys-import") {
            PrintMessage(as_json, "validator." + op, layer, "ok");
            return 0;
        }
    }

    if (command == "submit-commitment" && (layer == "l2" || layer == "l3")) {
        const std::string encoded = FindValue(argc, argv, "--commitment=");
        if (encoded.empty()) {
            std::cerr << "missing --commitment argument" << std::endl;
            return 1;
        }
        pantheon::common::Commitment commitment{};
        auto result = pantheon::common::DecodeCommitment(encoded, commitment);
        if (!result.valid) {
            std::cerr << "commitment decode error: " << result.reason << std::endl;
            return 1;
        }
        PrintMessage(as_json, "submit-commitment", layer, "commitment accepted");
        return 0;
    }

    PrintUsage();
    return 1;
}
