#include "../../../layer1-talanton/settlement/destination_tag.h"
#include "../../../layer1-talanton/settlement/escrow.h"
#include "../../../layer1-talanton/settlement/multisig.h"

#include "../../../layer1-talanton/core/crypto/sha256.h"

#include <cassert>
#include <cstring>
#include <iostream>

using namespace parthenon::settlement;

void test_destination_tag() {
    std::cout << "Testing destination tags..." << std::endl;

    // Test basic tag creation
    DestinationTag tag1(12345);
    assert(tag1.GetTag() == 12345);
    assert(tag1.GetMemo().empty());
    assert(tag1.IsValid());

    // Test tag with memo
    DestinationTag tag2(67890, "Payment for invoice #1234");
    assert(tag2.GetTag() == 67890);
    assert(tag2.GetMemo() == "Payment for invoice #1234");
    assert(tag2.IsValid());

    // Test serialization
    auto serialized = tag2.Serialize();
    size_t pos = 0;
    DestinationTag tag3 = DestinationTag::Deserialize(serialized, pos);
    assert(tag3 == tag2);
    assert(pos == serialized.size());

    // Test tag validation
    std::string long_memo(300, 'x'); // Too long
    DestinationTag invalid_tag(1, long_memo);
    assert(!invalid_tag.IsValid());

    std::cout << "  ✓ Destination tag tests passed" << std::endl;
}

void test_time_lock_escrow() {
    std::cout << "Testing time-locked escrow..." << std::endl;

    // Create time-locked escrow (release after timestamp 1000000)
    TimeLockEscrow escrow(1000000);
    assert(escrow.GetLocktime() == 1000000);

    // Test release conditions
    assert(!escrow.IsReleasable(999999)); // Too early
    assert(escrow.IsReleasable(1000000)); // Exactly at locktime
    assert(escrow.IsReleasable(1000001)); // After locktime

    // Test serialization
    auto serialized = escrow.Serialize();
    size_t pos = 0;
    TimeLockEscrow escrow2 = TimeLockEscrow::Deserialize(serialized, pos);
    assert(escrow2.GetLocktime() == escrow.GetLocktime());

    std::cout << "  ✓ Time-locked escrow tests passed" << std::endl;
}

void test_hash_lock_escrow() {
    std::cout << "Testing hash-locked escrow..." << std::endl;

    // Create a preimage and hash it
    Preimage preimage;
    for (size_t i = 0; i < 32; ++i) {
        preimage[i] = static_cast<uint8_t>(i * 7 + 13);
    }

    // Hash the preimage with SHA-256 as required by HTLC/hash-lock semantics
    auto sha_result = parthenon::crypto::SHA256::Hash256(preimage.data(), preimage.size());
    Hash256 hash;
    std::copy(sha_result.begin(), sha_result.end(), hash.begin());

    HashLockEscrow escrow(hash);
    assert(escrow.GetHash() == hash);

    // Test serialization
    auto serialized = escrow.Serialize();
    size_t pos = 0;
    HashLockEscrow escrow2 = HashLockEscrow::Deserialize(serialized, pos);
    assert(escrow2.GetHash() == hash);

    std::cout << "  ✓ Hash-locked escrow tests passed" << std::endl;
}

void test_conditional_escrow() {
    std::cout << "Testing conditional escrow..." << std::endl;

    Hash256 hash;
    hash.fill(0x42);

    ConditionalEscrow escrow(2000000, hash);
    assert(escrow.GetLocktime() == 2000000);
    assert(escrow.GetHash() == hash);

    // Test serialization
    auto serialized = escrow.Serialize();
    size_t pos = 0;
    ConditionalEscrow escrow2 = ConditionalEscrow::Deserialize(serialized, pos);
    assert(escrow2.GetLocktime() == escrow.GetLocktime());
    assert(escrow2.GetHash() == hash);

    std::cout << "  ✓ Conditional escrow tests passed" << std::endl;
}

void test_escrow_container() {
    std::cout << "Testing escrow container..." << std::endl;

    // Test time-locked escrow
    Escrow escrow1(EscrowType::TIME_LOCKED);
    escrow1.SetTimeLock(TimeLockEscrow(1500000));
    assert(escrow1.GetType() == EscrowType::TIME_LOCKED);
    assert(escrow1.GetTimeLock() != nullptr);
    assert(escrow1.GetTimeLock()->GetLocktime() == 1500000);

    // Test is releasable
    assert(!escrow1.IsReleasable(1499999));
    assert(escrow1.IsReleasable(1500000));

    // Test serialization
    auto serialized = escrow1.Serialize();
    size_t pos = 0;
    Escrow escrow2 = Escrow::Deserialize(serialized, pos);
    assert(escrow2.GetType() == EscrowType::TIME_LOCKED);
    assert(escrow2.GetTimeLock()->GetLocktime() == 1500000);

    std::cout << "  ✓ Escrow container tests passed" << std::endl;
}

void test_multisig_policy() {
    std::cout << "Testing multisig policy..." << std::endl;

    // Create some test public keys
    std::vector<PubKey> pubkeys;
    for (int i = 0; i < 3; ++i) {
        PubKey key;
        key.fill(static_cast<uint8_t>(i + 1));
        pubkeys.push_back(key);
    }

    // Create 2-of-3 multisig policy
    MultisigPolicy policy(2, pubkeys);
    assert(policy.GetM() == 2);
    assert(policy.GetN() == 3);
    assert(policy.IsValid());

    // Test invalid policies
    MultisigPolicy invalid1(0, pubkeys); // M = 0
    assert(!invalid1.IsValid());

    MultisigPolicy invalid2(4, pubkeys); // M > N
    assert(!invalid2.IsValid());

    // Test serialization
    auto serialized = policy.Serialize();
    size_t pos = 0;
    MultisigPolicy policy2 = MultisigPolicy::Deserialize(serialized, pos);
    assert(policy2.GetM() == 2);
    assert(policy2.GetN() == 3);
    assert(policy2.IsValid());

    std::cout << "  ✓ Multisig policy tests passed" << std::endl;
}

void test_aggregated_signature() {
    std::cout << "Testing aggregated signature..." << std::endl;

    AggregatedSignature agg_sig;

    // Add some signatures
    Signature sig1, sig2;
    sig1.fill(0x11);
    sig2.fill(0x22);

    agg_sig.AddSignature(0, sig1);
    agg_sig.AddSignature(2, sig2);

    assert(agg_sig.GetSignatureCount() == 2);
    assert(agg_sig.HasSignature(0));
    assert(!agg_sig.HasSignature(1));
    assert(agg_sig.HasSignature(2));

    // Test serialization
    auto serialized = agg_sig.Serialize();
    size_t pos = 0;
    AggregatedSignature agg_sig2 = AggregatedSignature::Deserialize(serialized, pos);
    assert(agg_sig2.GetSignatureCount() == 2);
    assert(agg_sig2.HasSignature(0));
    assert(agg_sig2.HasSignature(2));

    std::cout << "  ✓ Aggregated signature tests passed" << std::endl;
}

int main() {
    std::cout << "Running settlement module tests..." << std::endl;
    std::cout << std::endl;

    try {
        test_destination_tag();
        test_time_lock_escrow();
        test_hash_lock_escrow();
        test_conditional_escrow();
        test_escrow_container();
        test_multisig_policy();
        test_aggregated_signature();

        std::cout << std::endl;
        std::cout << "================================" << std::endl;
        std::cout << "All settlement tests PASSED! ✓" << std::endl;
        std::cout << "================================" << std::endl;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
