// Copyright 2017 Altronix Corp.
// This file is part of the tiny-ether library
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

/**
 * @author Thomas Chiantia <thomas@altronix>
 * @date 2017
 */

#include "rlpx_frame.h"
#include "rlpx_handshake.h"
#include "rlpx_helper_macros.h"
#include "rlpx_io.h"
#include "ukeccak256.h"
extern rlpx_devp2p_protocol_settings g_devp2p_settings;
/*
 * This "test" feature is only to export data from normally opaque structures.
 */

uecc_ctx*
rlpx_test_skey(rlpx_io* ch)
{
    return ch->skey;
}

uecc_ctx*
rlpx_test_ekey(rlpx_io* ch)
{
    return &ch->ekey;
}

void
rlpx_test_nonce_set(rlpx_io* s, h256* nonce)
{
    memcpy(s->nonce.b, nonce->b, 32);
}

void
rlpx_test_ekey_set(rlpx_io* s, uecc_ctx* ekey)
{
    uecc_key_deinit(&s->ekey);
    s->ekey = *ekey;
}

ukeccak256_ctx*
rlpx_test_ingress(rlpx_io* ch)
{
    return &ch->x.imac;
}

ukeccak256_ctx*
rlpx_test_egress(rlpx_io* ch)
{
    return &ch->x.emac;
}

uaes_ctx*
rlpx_test_aes_mac(rlpx_io* ch)
{
    return &ch->x.aes_mac;
}

uaes_ctx*
rlpx_test_aes_enc(rlpx_io* ch)
{
    return &ch->x.aes_enc;
}

uaes_ctx*
rlpx_test_aes_dec(rlpx_io* ch)
{
    return &ch->x.aes_dec;
}

int
rlpx_test_expect_secrets(
    rlpx_io* s,
    int orig,
    uint8_t* sent,
    uint32_t slen,
    uint8_t* recv,
    uint32_t rlen,
    uint8_t* aes,
    uint8_t* mac,
    uint8_t* foo)
{
    int err;
    uint8_t buf[32 + ((slen > rlen) ? slen : rlen)], *out = &buf[32];
    if ((err = uecc_agree(&s->ekey, &s->hs->ekey_remote))) return err;
    memcpy(buf, orig ? s->hs->nonce_remote.b : s->nonce.b, 32);
    memcpy(out, orig ? s->nonce.b : s->hs->nonce_remote.b, 32);

    // aes-secret / mac-secret
    ukeccak256(buf, 64, out, 32);          // h(nonces)
    memcpy(buf, &s->ekey.z.b[1], 32);      // (ephemeral || h(nonces))
    ukeccak256(buf, 64, out, 32);          // S(ephemeral || H(nonces))
    ukeccak256(buf, 64, out, 32);          // S(ephemeral || H(shared))
    uaes_init_bin(&s->x.aes_enc, out, 32); // aes-secret save
    uaes_init_bin(&s->x.aes_dec, out, 32); // aes-secret save
    if (memcmp(out, aes, 32)) return -1;   // test
    ukeccak256(buf, 64, out, 32);          // S(ephemeral || H(aes-secret))
    uaes_init_bin(&s->x.aes_mac, out, 32); // mac-secret save
    if (memcmp(out, mac, 32)) return -1;   // test

    // ingress / egress
    ukeccak256_init(&s->x.emac);
    ukeccak256_init(&s->x.imac);
    XOR32_SET(buf, out, s->nonce.b); // (mac-secret^recepient-nonce);
    memcpy(&buf[32], recv, rlen);    // (m..^nonce)||auth-recv-init)
    ukeccak256_update(&s->x.imac, buf, 32 + rlen); // S(m..^nonce)||auth-recv)
    XOR32(buf, s->nonce.b);                        // UNDO xor
    XOR32(buf, s->hs->nonce_remote.b);             // (mac-secret^nonce);
    memcpy(&buf[32], sent, slen); // (m..^nonce)||auth-sentd-init)
    ukeccak256_update(&s->x.emac, buf, 32 + slen); // S(m..^nonce)||auth-sent)

    // test foo
    if (foo) {
        if (orig) {
            ukeccak256_update(&s->x.emac, (uint8_t*)"foo", 3);
            ukeccak256_digest(&s->x.emac, out);
        } else {
            ukeccak256_update(&s->x.imac, (uint8_t*)"foo", 3);
            ukeccak256_digest(&s->x.imac, out);
        }
        if (memcmp(out, foo, 32)) return -1;
    }

    return err;
}

void
rlpx_test_mock_devp2p(rlpx_devp2p_protocol_settings* settings)
{
    g_devp2p_settings = *settings;
}
//
//
//
