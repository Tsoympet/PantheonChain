#include "storage/block_storage.h"

#include "primitives/asset.h"
#include "primitives/transaction.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <unistd.h>

using namespace parthenon;

namespace {

primitives::Block MakeTestBlock() {
    primitives::Block block;
    block.header.version = 2;
    block.header.timestamp = 1704067200;
    block.header.bits = 0x1d00ffff;
    block.header.nonce = 12345;

    primitives::Transaction coinbase;
    coinbase.version = 1;
    coinbase.locktime = 0;

    primitives::TxInput input;
    input.prevout = primitives::OutPoint(std::array<uint8_t, 32>{}, primitives::COINBASE_VOUT_INDEX);
    input.signature_script = {0x51, 0x21, 0x02};
    coinbase.inputs.push_back(input);

    coinbase.outputs.emplace_back(primitives::AssetID::TALANTON,
                                  50ULL * primitives::AssetSupply::BASE_UNIT,
                                  std::vector<uint8_t>{0x51});

    block.transactions.push_back(coinbase);
    block.header.merkle_root = block.CalculateMerkleRoot();
    return block;
}

}  // namespace

int main() {
    std::cout << "=== BlockStorage Tests ===" << std::endl;

    storage::BlockStorage block_storage;

    const auto base = std::filesystem::temp_directory_path();
    const auto db_path = base / ("parthenon_block_storage_test_" + std::to_string(::getpid()));
    std::filesystem::remove_all(db_path);

    assert(block_storage.Open(db_path.string()));

    const auto block = MakeTestBlock();
    const auto hash = block.GetHash();

    assert(block_storage.StoreBlock(block, 1));
    assert(block_storage.UpdateChainTip(1, hash));
    assert(block_storage.GetHeight() == 1);

    auto by_height = block_storage.GetBlockByHeight(1);
    assert(by_height.has_value());
    assert(!by_height->transactions.empty());
    assert(by_height->Serialize() == block.Serialize());

    auto by_hash = block_storage.GetBlockByHash(hash);
    assert(by_hash.has_value());
    assert(by_hash->Serialize() == block.Serialize());

    block_storage.Close();
    std::filesystem::remove_all(db_path);

    std::cout << "\n✓ All block storage tests passed!" << std::endl;
    return 0;
}
