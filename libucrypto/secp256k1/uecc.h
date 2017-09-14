#ifndef DH_H_
#define DH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "secp256k1.h"
#include "secp256k1_ecdh.h"
#include "secp256k1_recovery.h"

typedef unsigned char byte;

// clang-format off
typedef struct { byte b[256] ;} h2048;
typedef struct { byte b[128] ;} h1024;
typedef struct { byte b[65]  ;} h520;
typedef struct { byte b[64]  ;} h512;
typedef struct { byte b[32]  ;} h256;
typedef struct { byte b[20]  ;} h160;
typedef struct { byte b[16]  ;} h128;
typedef struct { byte b[8]   ;} h64;
// clang-format on

/*!< r: [0, 32), s: [32, 64), v: 64 */
typedef h520 uecc_signature;
typedef h520 uecc_public_key_recoverable;
typedef secp256k1_pubkey uecc_public_key;
typedef h256 uecc_private_key;
typedef h256 uecc_shared_secret;

typedef struct
{
    secp256k1_context* grp; /*!< lib export */
    uecc_private_key d;     /*!< private key */
    uecc_public_key Q;      /*!< public key */
    uecc_public_key Qp;     /*!< remote public key */
    uecc_shared_secret z;   /*!< shared secret */
} uecc_ctx;

#define uecc_z_cmp(x, y) uecc_point_cmp(x, y)

/**
 * @brief initialize a key context
 *
 * @param uecc_ctx
 *
 * @return
 */
int uecc_key_init(uecc_ctx*, const uecc_private_key* d);
int uecc_key_init_binary(uecc_ctx*, const uecc_private_key* d);
int uecc_key_init_string(uecc_ctx* ctx, int radix, const char*);
int uecc_key_init_new(uecc_ctx*);

/**
 * @brief
 *
 * @param uecc_ctx
 */
void uecc_key_deinit(uecc_ctx*);

/**
 * @brief
 *
 * @param ctx
 * @param k
 *
 * @return
 */
int uecc_agree(uecc_ctx* ctx, const uecc_public_key* k);

/**
 * @brief
 *
 * @param ctx
 * @param b
 * @param sz
 * @param uecc_signature
 *
 * @return
 */
int uecc_sign(uecc_ctx* ctx, const byte* b, size_t sz, uecc_signature*);

/**
 * @brief
 *
 * @param q
 * @param b
 * @param sz
 * @param uecc_signature
 *
 * @return
 */
int uecc_verify(const uecc_public_key* q,
                const byte* b,
                size_t sz,
                uecc_signature*);

#ifdef __cplusplus
}
#endif
#endif
