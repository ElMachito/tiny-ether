#ifndef SHA_H_
#define SHA_H_
#ifdef __cplusplus
extern "C" {
#endif

#define ucrypto_sha3_256(b, l, d) sha3_256(b, l, d)

typedef uint8_t ucrypto_h256[32];

#ifdef __cplusplus
}
#endif
#endif