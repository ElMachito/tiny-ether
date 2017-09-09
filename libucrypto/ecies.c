#include "ecies.h"

int
ucrypto_ecies_decrypt(ucrypto_ecc_ctx* secret,
                      uint8_t* cipher,
                      size_t cipher_len,
                      uint8_t* plain,
                      size_t plain_len)
{
    // TODO: Encryption
    // 0x04 + echd-random-pubk + iv + aes(kdf(shared-secret), plaintext) + hmac
    // * offset 0                65         81               275
    // *        [ecies-pubkey:65||aes-iv:16||cipher-text:194||ecies-mac:32]
    int err = 0;
    err = ucrypto_ecc_agree(secret, (ucrypto_ecc_public_key*)cipher);
    ((void)cipher_len);
    ((void)plain);
    ((void)plain_len);
    return err;
}
