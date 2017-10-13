/*
 * @file rlpx_handshake.c
 *
 * @brief
 *
 * AUTH - legacy
 * E(remote-pub,S(eph,s-shared^nonce) || H(eph-pub) || pub || nonce || 0x0)
 *
 * ACK - legacy
 * E(remote-pub, remote-ephemeral || nonce || 0x0)
 */

#include "rlpx_handshake.h"
#include "rlpx_helper_macros.h"
#include "uecies_decrypt.h"
#include "uecies_encrypt.h"
#include "ukeccak256.h"
#include "unonce.h"
#include "urand.h"

// rlp <--> cipher text
int rlpx_encrypt(urlp* rlp, const uecc_public_key* q, uint8_t*, size_t* l);
int rlpx_decrypt(uecc_ctx* ctx, const uint8_t* cipher, size_t l, urlp** rlp);

// legacy methods
int rlpx_auth_read_legacy(uecc_ctx* skey,
                          const uint8_t* auth,
                          size_t l,
                          urlp** rlp_p);
int rlpx_ack_read_legacy(uecc_ctx* skey,
                         const uint8_t* auth,
                         size_t l,
                         urlp** rlp_p);
int
rlpx_encrypt(urlp* rlp, const uecc_public_key* q, uint8_t* p, size_t* l)
{
    int err;

    // plain text size
    uint32_t rlpsz = urlp_print_size(rlp);
    size_t padsz = urand_min_max_u8(RLPX_MIN_PAD, RLPX_MAX_PAD);

    // cipher prefix big endian
    uint16_t sz = uecies_encrypt_size(padsz + rlpsz) + sizeof(uint16_t);

    // Dynamic stack buffer for plain text
    uint8_t plain[rlpsz + padsz], *psz = (uint8_t *)&sz;

    // endian test
    static int x = 1;
    *(uint16_t*)p = *(uint8_t*)&x ? (psz[0] << 8 | psz[1]) : *(uint16_t*)psz;

    // Is caller buffer big enough?
    if (!(sz <= *l)) {
        *l = sz;
        return -1;
    }

    // Inform caller size, print and encrypt rlp
    *l = sz;
    if (urlp_print(rlp, plain, &rlpsz)) return -1;
    urand(&plain[rlpsz], padsz);
    err = uecies_encrypt(q, p, 2, plain, padsz + rlpsz, &p[2]);
    return err;
}

int
rlpx_decrypt(uecc_ctx* ecc, const uint8_t* c, size_t l, urlp** rlp_p)
{
    // endian test
    static int x = 1;
    uint16_t sz = *(uint8_t*)&x ? (c[0] << 8 | c[1]) : *(uint16_t*)c;

    // Dynamic stack buffer for cipher text
    uint8_t buffer[sz];

    // Decrypt and parse rlp
    int err = uecies_decrypt(ecc, c, 2, &c[2], l - 2, buffer);
    return ((err > 0) && (*rlp_p = urlp_parse(buffer, err))) ? 0 : -1;
}

int
rlpx_auth_read(uecc_ctx* skey, const uint8_t* auth, size_t l, urlp** rlp_p)
{
    int err = rlpx_decrypt(skey, auth, l, rlp_p);
    if (err) err = rlpx_auth_read_legacy(skey, auth, l, rlp_p);
    return err;
}

int
rlpx_auth_read_legacy(uecc_ctx* skey,
                      const uint8_t* auth,
                      size_t l,
                      urlp** rlp_p)
{
    int err = -1;
    uint8_t b[194];
    uint64_t v = 4;
    if (!(l == 307)) return err;
    if (!(uecies_decrypt(skey, NULL, 0, auth, l, b) == 194)) return err;
    if (!(*rlp_p = urlp_list())) return err;
    urlp_push(*rlp_p, urlp_item_u8(b, 65));                // signature
    urlp_push(*rlp_p, urlp_item_u8(&b[65 + 32], 64));      // pubkey
    urlp_push(*rlp_p, urlp_item_u8(&b[65 + 32 + 64], 32)); // nonce
    urlp_push(*rlp_p, urlp_item_u64(&v, 1));               // version
    return 0;
}

int
rlpx_auth_load(uecc_ctx* skey,
               uint64_t* remote_version,
               h256* remote_nonce,
               uecc_public_key* remote_spub,
               uecc_public_key* remote_epub,
               urlp** rlp_p)
{
    int err = -1;
    uint8_t buffer[65];
    urlp* rlp = *rlp_p;
    const urlp* seek;
    if ((seek = urlp_at(rlp, 3))) {
        // Get version
        *remote_version = urlp_as_u64(seek);
    }
    if ((seek = urlp_at(rlp, 2)) && urlp_size(seek) == sizeof(h256)) {
        // Read remote nonce
        memcpy(remote_nonce->b, urlp_ref(seek, NULL), sizeof(h256));
    }
    if ((seek = urlp_at(rlp, 1)) &&
        urlp_size(seek) == sizeof(uecc_public_key)) {
        // Get secret from remote public key
        buffer[0] = 0x04;
        memcpy(&buffer[1], urlp_ref(seek, NULL), urlp_size(seek));
        uecc_btoq(buffer, 65, remote_spub);
        uecc_agree(skey, remote_spub);
    }
    if ((seek = urlp_at(rlp, 0)) &&
        // Get remote ephemeral public key from signature
        urlp_size(seek) == sizeof(uecc_signature)) {
        uecc_shared_secret x;
        XOR32_SET(x.b, (&skey->z.b[1]), remote_nonce->b);
        err = uecc_recover_bin(urlp_ref(seek, NULL), &x, remote_epub);
    }
    // urlp_free(&rlp);
    return err;
}

int
rlpx_auth_write(uecc_ctx* skey,
                uecc_ctx* ekey,
                h256* nonce,
                const uecc_public_key* to_s_key,
                uint8_t* auth,
                size_t* l)
{
    int err = 0;
    uint64_t v = 4;
    uint8_t rawsig[65];
    uint8_t rawpub[65];
    uecc_shared_secret x;
    uecc_signature sig;
    urlp* rlp;
    if (uecc_agree(skey, to_s_key)) return -1;
    for (int i = 0; i < 32; i++) {
        x.b[i] = skey->z.b[i + 1] ^ nonce->b[i];
    }
    if (uecc_sign(ekey, x.b, 32, &sig)) return -1;
    uecc_sig_to_bin(&sig, rawsig);
    uecc_qtob(&skey->Q, rawpub, 65);
    if ((rlp = urlp_list())) {
        urlp_push(rlp, urlp_item_u8(rawsig, 65));
        urlp_push(rlp, urlp_item_u8(&rawpub[1], 64));
        urlp_push(rlp, urlp_item_u8(nonce->b, 32));
        urlp_push(rlp, urlp_item_u64(&v, 1));
    }
    err = rlpx_encrypt(rlp, to_s_key, auth, l);
    urlp_free(&rlp);
    return err;
}

int
rlpx_ack_read(uecc_ctx* skey, const uint8_t* ack, size_t l, urlp** rlp_p)
{
    int err = rlpx_decrypt(skey, ack, l, rlp_p);
    if (err) err = rlpx_ack_read_legacy(skey, ack, l, rlp_p);
    return err;
}

int
rlpx_ack_read_legacy(uecc_ctx* skey,
                     const uint8_t* auth,
                     size_t l,
                     urlp** rlp_p)
{
    int err = -1;
    uint8_t b[194];
    uint64_t v = 4;
    if (!(uecies_decrypt(skey, NULL, 0, auth, l, b) > 0)) return err;
    if (!(*rlp_p = urlp_list())) return err;
    urlp_push(*rlp_p, urlp_item_u8(b, 64));      // pubkey
    urlp_push(*rlp_p, urlp_item_u8(&b[64], 32)); // nonce
    urlp_push(*rlp_p, urlp_item_u64(&v, 1));     // ver
    return 0;
}

int
rlpx_ack_load(uint64_t* remote_version,
              h256* remote_nonce,
              uecc_public_key* remote_ekey,
              urlp** rlp_p)
{
    int err = -1;
    uint8_t buff[65];
    const urlp* seek;
    urlp* rlp = *rlp_p;
    if ((seek = urlp_at(rlp, 0)) &&
        (urlp_size(seek) == sizeof(uecc_public_key))) {
        buff[0] = 0x04;
        memcpy(&buff[1], urlp_ref(seek, NULL), urlp_size(seek));
        uecc_btoq(buff, 65, remote_ekey);
    }
    if ((seek = urlp_at(rlp, 1)) && (urlp_size(seek) == sizeof(h256))) {
        memcpy(remote_nonce->b, urlp_ref(seek, NULL), sizeof(h256));
        err = 0;
    }
    if ((seek = urlp_at(rlp, 2))) *remote_version = urlp_as_u64(seek);
    return err;
}

int
rlpx_ack_write(uecc_ctx* skey,
               uecc_ctx* ekey,
               h256* nonce,
               const uecc_public_key* to_s_key,
               uint8_t* auth,
               size_t* l)
{
    h520 rawekey;
    urlp* rlp;
    uint64_t ver = 4;
    int err = 0;
    if (uecc_qtob(&ekey->Q, rawekey.b, sizeof(rawekey.b))) return -1;
    if (!(rlp = urlp_list())) return -1;
    if (rlp) {
        urlp_push(rlp, urlp_item_u8(&rawekey.b[1], 64));
        urlp_push(rlp, urlp_item_u8(nonce->b, 32));
        urlp_push(rlp, urlp_item_u64(&ver, 1));
    }
    if (!(urlp_children(rlp) == 3)) {
        urlp_free(&rlp);
        return -1;
    }

    err = rlpx_encrypt(rlp, to_s_key, auth, l);
    urlp_free(&rlp);
    return err;
}

rlpx_handshake*
rlpx_handshake_alloc_auth(uecc_ctx* skey,
                          uecc_ctx* ekey,
                          uint64_t* version_remote,
                          h256* nonce,
                          h256* nonce_remote,
                          uecc_public_key* skey_remote,
                          uecc_public_key* ekey_remote,
                          const uecc_public_key* to)
{
    rlpx_handshake* hs =
        rlpx_handshake_alloc(skey, ekey, version_remote, nonce, nonce_remote,
                             skey_remote, ekey_remote);
    if (hs) rlpx_handshake_auth_init(hs, nonce, to);
    return hs;
}

rlpx_handshake*
rlpx_handshake_alloc(uecc_ctx* skey,
                     uecc_ctx* ekey,
                     uint64_t* version_remote,
                     h256* nonce,
                     h256* nonce_remote,
                     uecc_public_key* skey_remote,
                     uecc_public_key* ekey_remote)
{
    rlpx_handshake* hs = rlpx_malloc(sizeof(rlpx_handshake));
    if (hs) {
        hs->cipher_len = sizeof(hs->cipher);
        hs->cipher_remote_len = sizeof(hs->cipher_remote);
        hs->ekey = ekey;
        hs->skey = skey;
        hs->nonce = nonce;
        hs->nonce_remote = nonce_remote;
        hs->skey_remote = skey_remote;
        hs->ekey_remote = ekey_remote;
        hs->version_remote = version_remote;
    }
    return hs;
}

void
rlpx_handshake_free(rlpx_handshake** hs_p)
{
    rlpx_handshake* hs = *hs_p;
    *hs_p = NULL;
    memset(hs, 0, sizeof(rlpx_handshake));
    rlpx_free(hs);
}

int
rlpx_handshake_auth_init(rlpx_handshake* hs,
                         h256* nonce,
                         const uecc_public_key* to)
{

    int err = 0;
    uint64_t v = 4;
    uint8_t rawsig[65];
    uint8_t rawpub[65];
    uecc_shared_secret x;
    uecc_signature sig;
    urlp* rlp;
    if (uecc_agree(hs->skey, to)) return -1;
    for (int i = 0; i < 32; i++) {
        x.b[i] = hs->skey->z.b[i + 1] ^ nonce->b[i];
    }
    if (uecc_sign(hs->ekey, x.b, 32, &sig)) return -1;
    uecc_sig_to_bin(&sig, rawsig);
    uecc_qtob(&hs->skey->Q, rawpub, 65);
    if ((rlp = urlp_list())) {
        urlp_push(rlp, urlp_item_u8(rawsig, 65));
        urlp_push(rlp, urlp_item_u8(&rawpub[1], 64));
        urlp_push(rlp, urlp_item_u8(nonce->b, 32));
        urlp_push(rlp, urlp_item_u64(&v, 1));
    }
    err = rlpx_encrypt(rlp, to, hs->cipher, &hs->cipher_len);
    if (!err) *hs->nonce = *nonce;
    urlp_free(&rlp);
    return err;
}

int
rlpx_handshake_auth_read(rlpx_handshake* hs,
                         const uint8_t* b,
                         size_t l,
                         urlp** rlp_p)
{
    int err = rlpx_decrypt(hs->skey, b, l, rlp_p);
    memcpy(hs->cipher_remote, b, l);
    hs->cipher_remote_len = l;
    if (err) err = rlpx_handshake_auth_read_legacy(hs, b, l, rlp_p);
    return err;
}

int
rlpx_handshake_auth_read_legacy(rlpx_handshake* hs,
                                const uint8_t* auth,
                                size_t l,
                                urlp** rlp_p)
{
    int err = -1;
    uint8_t b[194];
    uint64_t v = 4;
    if (!(l == 307)) return err;
    if (!(uecies_decrypt(hs->skey, NULL, 0, auth, l, b) == 194)) return err;
    if (!(*rlp_p = urlp_list())) return err;
    urlp_push(*rlp_p, urlp_item_u8(b, 65));                // signature
    urlp_push(*rlp_p, urlp_item_u8(&b[65 + 32], 64));      // pubkey
    urlp_push(*rlp_p, urlp_item_u8(&b[65 + 32 + 64], 32)); // nonce
    urlp_push(*rlp_p, urlp_item_u64(&v, 1));               // version
    return 0;
}

//
//
//
//
