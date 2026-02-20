#include <iostream>
#include <string>

namespace {

void PrintUsage() {
    std::cerr << "Usage: pantheon-node --layer=l1|l2|l3\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string layer;
#ifdef PANTHEON_NODE_LAYER
    layer = PANTHEON_NODE_LAYER;
#endif
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg.rfind("--layer=", 0) == 0) {
            layer = arg.substr(8);
        } else {
            PrintUsage();
            return 1;
        }
    }

    if (layer != "l1" && layer != "l2" && layer != "l3") {
        PrintUsage();
        return 1;
    }

    if (layer == "l1") {
        std::cout << "Starting TALANTON L1 node (PoW settlement/security anchor)" << std::endl;
    } else if (layer == "l2") {
        std::cout << "Starting DRACHMA L2 node (PoS payments/liquidity)" << std::endl;
    } else {
        std::cout << "Starting OBOLOS L3 node (PoS EVM execution)" << std::endl;
    }

    return 0;
}
