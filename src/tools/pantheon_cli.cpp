#include "common/serialization.h"

#include <iostream>
#include <string>

namespace {

void PrintUsage() {
    std::cerr << "Usage:\n"
              << "  pantheon-cli stake deposit --layer=l2\n"
              << "  pantheon-cli deploy-contract --layer=l3\n"
              << "  pantheon-cli submit-commitment --layer=l2|l3 --commitment=<encoded>\n";
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

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    const std::string command = argv[1];
    const std::string layer = FindValue(argc, argv, "--layer=");

    if (command == "stake" && argc >= 3 && std::string(argv[2]) == "deposit" && layer == "l2") {
        std::cout << "staking deposit accepted on DRACHMA" << std::endl;
        return 0;
    }

    if (command == "deploy-contract" && layer == "l3") {
        std::cout << "contract deployment request accepted on OBOLOS" << std::endl;
        return 0;
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
        std::cout << "commitment accepted for layer " << layer << std::endl;
        return 0;
    }

    PrintUsage();
    return 1;
}
