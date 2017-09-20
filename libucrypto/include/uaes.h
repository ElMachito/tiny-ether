#ifndef AES_H_
#define AES_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "mbedtls/aes.h"

typedef struct
{
    uint8_t b[16];
} uaes_128_ctr_key;

typedef struct
{
    uint8_t b[16];
} uaes_iv;

typedef mbedtls_aes_context uaes_ctx;

int uaes_init(uaes_ctx* ctx, uaes_128_ctr_key* key);
void uaes_deinit(uaes_ctx** ctx);

int uaes_crypt(uaes_128_ctr_key* key,
               uaes_iv* iv,
               const uint8_t* in,
               size_t inlen,
               uint8_t* out);

#ifdef __cplusplus
}
#endif
#endif
