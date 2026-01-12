// ParthenonChain - Asset ID
// Consensus-critical: Multi-asset UTXO system
// Three native assets: TALANTON (TALN), DRACHMA (DRM), OBOLOS (OBL)

#ifndef PARTHENON_PRIMITIVES_ASSET_H
#define PARTHENON_PRIMITIVES_ASSET_H

#include <cstdint>
#include <string>
#include <array>

namespace parthenon {
namespace primitives {

/**
 * AssetID represents one of the three native assets in ParthenonChain
 * 
 * TALANTON (TALN) - Primary currency, max supply 21,000,000
 * DRACHMA (DRM)   - Settlement asset, max supply 41,000,000
 * OBOLOS (OBL)    - Gas/smart contract asset, max supply 61,000,000
 * 
 * Consensus-critical: Asset IDs must never change
 */
enum class AssetID : uint8_t {
    TALANTON = 0,  // TALN - Primary currency
    DRACHMA = 1,   // DRM - Settlement asset
    OBOLOS = 2     // OBL - Gas/smart contract fuel
};

/**
 * Asset supply caps (enforced by consensus)
 * All amounts are in base units (like satoshis)
 */
class AssetSupply {
public:
    // Maximum supply for each asset (in base units)
    static constexpr uint64_t TALN_MAX_SUPPLY = 21000000ULL * 100000000ULL; // 21M TALN
    static constexpr uint64_t DRM_MAX_SUPPLY  = 41000000ULL * 100000000ULL; // 41M DRM
    static constexpr uint64_t OBL_MAX_SUPPLY  = 61000000ULL * 100000000ULL; // 61M OBL
    
    // Base unit divisor (8 decimals like Bitcoin)
    static constexpr uint64_t BASE_UNIT = 100000000ULL;
    
    /**
     * Get maximum supply for an asset
     */
    static uint64_t GetMaxSupply(AssetID asset) {
        switch (asset) {
            case AssetID::TALANTON:
                return TALN_MAX_SUPPLY;
            case AssetID::DRACHMA:
                return DRM_MAX_SUPPLY;
            case AssetID::OBOLOS:
                return OBL_MAX_SUPPLY;
            default:
                return 0;
        }
    }
    
    /**
     * Validate that an amount does not exceed asset maximum
     */
    static bool IsValidAmount(AssetID asset, uint64_t amount) {
        return amount <= GetMaxSupply(asset);
    }
    
    /**
     * Get asset name as string
     */
    static std::string GetAssetName(AssetID asset) {
        switch (asset) {
            case AssetID::TALANTON:
                return "TALANTON";
            case AssetID::DRACHMA:
                return "DRACHMA";
            case AssetID::OBOLOS:
                return "OBOLOS";
            default:
                return "UNKNOWN";
        }
    }
    
    /**
     * Get asset ticker symbol
     */
    static std::string GetAssetTicker(AssetID asset) {
        switch (asset) {
            case AssetID::TALANTON:
                return "TALN";
            case AssetID::DRACHMA:
                return "DRM";
            case AssetID::OBOLOS:
                return "OBL";
            default:
                return "???";
        }
    }
};

/**
 * AssetAmount combines an asset ID with an amount
 * Used in transaction outputs
 */
struct AssetAmount {
    AssetID asset;
    uint64_t amount;
    
    AssetAmount() : asset(AssetID::TALANTON), amount(0) {}
    AssetAmount(AssetID a, uint64_t amt) : asset(a), amount(amt) {}
    
    bool operator==(const AssetAmount& other) const {
        return asset == other.asset && amount == other.amount;
    }
    
    bool operator!=(const AssetAmount& other) const {
        return !(*this == other);
    }
    
    /**
     * Validate this asset amount
     */
    bool IsValid() const {
        return AssetSupply::IsValidAmount(asset, amount);
    }
    
    /**
     * Serialize to bytes (1 byte asset ID + 8 bytes amount)
     */
    void Serialize(uint8_t* output) const;
    
    /**
     * Deserialize from bytes
     */
    static AssetAmount Deserialize(const uint8_t* input);
};

} // namespace primitives
} // namespace parthenon

#endif // PARTHENON_PRIMITIVES_ASSET_H
