#include "secp256k1.h"

/* Stub implementation for builds without the real secp256k1 dependency.
 * This is NOT cryptographically secure and must not be used in production. */

#include <stdlib.h>
#include <string.h>

struct secp256k1_context {
    unsigned int flags;
};

static const unsigned char secp256k1_stub_curve_order[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0xBA, 0xAE, 0xDC, 0xE6, 0xAF, 0x48, 0xA0, 0x3B,
    0xBF, 0xD2, 0x5E, 0x8C, 0xD0, 0x36, 0x41, 0x41
};

static int secp256k1_stub_is_zero(const unsigned char* data) {
    for (size_t i = 0; i < 32; ++i) {
        if (data[i] != 0) {
            return 0;
        }
    }
    return 1;
}

static int secp256k1_stub_compare(const unsigned char* lhs, const unsigned char* rhs) {
    for (size_t i = 0; i < 32; ++i) {
        if (lhs[i] < rhs[i]) {
            return -1;
        }
        if (lhs[i] > rhs[i]) {
            return 1;
        }
    }
    return 0;
}

static void secp256k1_stub_pubkey_from_seckey(const unsigned char* seckey,
                                              unsigned char* pubkey32) {
    for (size_t i = 0; i < 32; ++i) {
        pubkey32[i] = (unsigned char)(seckey[i] ^ 0xA5);
    }
}

static void secp256k1_stub_signature(const unsigned char* pubkey32, const unsigned char* msg32,
                                     unsigned char* sig64) {
    for (size_t i = 0; i < 32; ++i) {
        sig64[i] = (unsigned char)(pubkey32[i] ^ msg32[i]);
        sig64[32 + i] = (unsigned char)(pubkey32[i] ^ msg32[31 - i]);
    }
}

secp256k1_context* secp256k1_context_create(unsigned int flags) {
    secp256k1_context* ctx = (secp256k1_context*)malloc(sizeof(secp256k1_context));
    if (ctx != NULL) {
        ctx->flags = flags;
    }
    return ctx;
}

void secp256k1_context_destroy(secp256k1_context* ctx) {
    free(ctx);
}

int secp256k1_ec_seckey_verify(const secp256k1_context* ctx, const unsigned char* seckey) {
    (void)ctx;
    if (seckey == NULL) {
        return 0;
    }
    if (secp256k1_stub_is_zero(seckey)) {
        return 0;
    }
    if (secp256k1_stub_compare(seckey, secp256k1_stub_curve_order) >= 0) {
        return 0;
    }
    return 1;
}

int secp256k1_keypair_create(const secp256k1_context* ctx, secp256k1_keypair* keypair,
                             const unsigned char* seckey) {
    if (keypair == NULL || seckey == NULL) {
        return 0;
    }
    if (!secp256k1_ec_seckey_verify(ctx, seckey)) {
        return 0;
    }
    memcpy(keypair->data, seckey, 32);
    return 1;
}

int secp256k1_keypair_xonly_pub(const secp256k1_context* ctx, secp256k1_xonly_pubkey* pubkey,
                                int* pk_parity, const secp256k1_keypair* keypair) {
    (void)ctx;
    if (pubkey == NULL || keypair == NULL) {
        return 0;
    }
    if (pk_parity != NULL) {
        *pk_parity = 0;
    }
    secp256k1_stub_pubkey_from_seckey(keypair->data, pubkey->data);
    return 1;
}

int secp256k1_xonly_pubkey_serialize(const secp256k1_context* ctx, unsigned char* output32,
                                     const secp256k1_xonly_pubkey* pubkey) {
    (void)ctx;
    if (output32 == NULL || pubkey == NULL) {
        return 0;
    }
    memcpy(output32, pubkey->data, 32);
    return 1;
}

int secp256k1_xonly_pubkey_parse(const secp256k1_context* ctx, secp256k1_xonly_pubkey* pubkey,
                                 const unsigned char* input32) {
    (void)ctx;
    if (pubkey == NULL || input32 == NULL) {
        return 0;
    }
    if (secp256k1_stub_is_zero(input32)) {
        return 0;
    }
    memcpy(pubkey->data, input32, 32);
    return 1;
}

int secp256k1_schnorrsig_sign32(const secp256k1_context* ctx, unsigned char* sig64,
                                const unsigned char* msg32, const secp256k1_keypair* keypair,
                                const unsigned char* aux_rand32) {
    unsigned char pubkey[32];
    (void)ctx;
    (void)aux_rand32;
    if (sig64 == NULL || msg32 == NULL || keypair == NULL) {
        return 0;
    }
    secp256k1_stub_pubkey_from_seckey(keypair->data, pubkey);
    secp256k1_stub_signature(pubkey, msg32, sig64);
    return 1;
}

int secp256k1_schnorrsig_verify(const secp256k1_context* ctx, const unsigned char* sig64,
                                const unsigned char* msg32, size_t msglen,
                                const secp256k1_xonly_pubkey* pubkey) {
    unsigned char expected[64];
    (void)ctx;
    if (sig64 == NULL || msg32 == NULL || pubkey == NULL) {
        return 0;
    }
    if (msglen != 32) {
        return 0;
    }
    secp256k1_stub_signature(pubkey->data, msg32, expected);
    return memcmp(expected, sig64, 64) == 0;
}
