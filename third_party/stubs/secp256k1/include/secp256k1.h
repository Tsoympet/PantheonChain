#ifndef SECP256K1_STUB_H
#define SECP256K1_STUB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SECP256K1_CONTEXT_SIGN 1u
#define SECP256K1_CONTEXT_VERIFY 2u

typedef struct secp256k1_context secp256k1_context;

typedef struct secp256k1_keypair {
    unsigned char data[32];
} secp256k1_keypair;

typedef struct secp256k1_xonly_pubkey {
    unsigned char data[32];
} secp256k1_xonly_pubkey;

secp256k1_context* secp256k1_context_create(unsigned int flags);
void secp256k1_context_destroy(secp256k1_context* ctx);

int secp256k1_ec_seckey_verify(const secp256k1_context* ctx, const unsigned char* seckey);

int secp256k1_keypair_create(const secp256k1_context* ctx, secp256k1_keypair* keypair,
                             const unsigned char* seckey);

int secp256k1_keypair_xonly_pub(const secp256k1_context* ctx, secp256k1_xonly_pubkey* pubkey,
                                int* pk_parity, const secp256k1_keypair* keypair);

int secp256k1_xonly_pubkey_serialize(const secp256k1_context* ctx, unsigned char* output32,
                                     const secp256k1_xonly_pubkey* pubkey);

int secp256k1_xonly_pubkey_parse(const secp256k1_context* ctx, secp256k1_xonly_pubkey* pubkey,
                                 const unsigned char* input32);

int secp256k1_schnorrsig_sign32(const secp256k1_context* ctx, unsigned char* sig64,
                                const unsigned char* msg32, const secp256k1_keypair* keypair,
                                const unsigned char* aux_rand32);

int secp256k1_schnorrsig_verify(const secp256k1_context* ctx, const unsigned char* sig64,
                                const unsigned char* msg32, size_t msglen,
                                const secp256k1_xonly_pubkey* pubkey);

#ifdef __cplusplus
}
#endif

#endif
