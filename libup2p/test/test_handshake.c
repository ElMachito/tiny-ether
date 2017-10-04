#include "test.h"
#include "rlpx_test_helpers.h"

extern test_vector g_test_vectors[];
extern const char* g_alice_epub;
extern const char* g_bob_epub;
extern const char* g_aes_secret;
extern const char* g_mac_secret;
extern const char* g_foo;

int test_read();
int test_write();
int test_secrets();

int
test_handshake()
{
    int err = 0;
    err |= test_read();
    err |= test_write();
    err |= test_secrets();

    return err;
}

int
test_read()
{
    int err;
    test_session s;
    test_vector* tv = g_test_vectors;

    int i = 0;
    while (tv->auth) {
        test_session_init(&s, i);
        rlpx_test_remote_ekey_clr(s.alice);
        rlpx_test_remote_ekey_clr(s.bob);
        if (rlpx_ch_auth_load(s.bob, s.auth, s.authlen)) break;
        if (rlpx_ch_ack_load(s.alice, s.ack, s.acklen)) break;
        if (!(rlpx_ch_version_remote(s.bob) == tv->authver)) break;
        if (!(rlpx_ch_version_remote(s.alice) == tv->ackver)) break;
        if ((cmp_q(rlpx_ch_remote_pub_ekey(s.bob), rlpx_ch_pub_ekey(s.alice))))
            break;
        if ((cmp_q(rlpx_ch_remote_pub_ekey(s.alice), rlpx_ch_pub_ekey(s.bob))))
            break;
        test_session_deinit(&s);
        i++;
        tv++;
    }
    err = tv->auth ? -1 : 0; // broke loop early ? -> error
    return err;
}

int
test_write()
{
    int err;
    size_t l = 800; // ecies+pad
    uint8_t buf[l];
    test_session s;
    test_session_init(&s, 0);
    h256 nonce_a, nonce_b;

    unonce(nonce_a.b);
    unonce(nonce_b.b);
    rlpx_test_nonce_set(s.alice, &nonce_a);
    rlpx_test_nonce_set(s.bob, &nonce_b);

    IF_ERR_EXIT(rlpx_auth_write(rlpx_test_skey(s.alice),
                                rlpx_test_ekey(s.alice), &nonce_a,
                                rlpx_ch_pub_skey(s.bob), buf, &l));
    IF_ERR_EXIT(rlpx_ch_auth_load(s.bob, buf, l));

    l = 800;
    IF_ERR_EXIT(rlpx_ack_write(rlpx_test_skey(s.bob), rlpx_test_ekey(s.bob),
                               &nonce_b, rlpx_ch_pub_skey(s.alice), buf, &l));
    IF_ERR_EXIT(rlpx_ch_ack_load(s.alice, buf, l));

    IF_ERR_EXIT(check_q(rlpx_ch_remote_pub_ekey(s.alice), g_bob_epub));
    IF_ERR_EXIT(check_q(rlpx_ch_remote_pub_ekey(s.bob), g_alice_epub));
EXIT:
    test_session_deinit(&s);
    return err;
}

int
test_secrets()
{
    int err;
    test_session s;
    test_session_init(&s, 1);
    uint8_t aes[32], mac[32], foo[32];
    memcpy(aes, makebin(g_aes_secret, NULL), 32);
    memcpy(mac, makebin(g_mac_secret, NULL), 32);
    memcpy(foo, makebin(g_foo, NULL), 32);

    // Set some phoney nonces to read expected secrets
    rlpx_test_nonce_set(s.bob, &s.bob_n);
    rlpx_test_nonce_set(s.alice, &s.alice_n);
    rlpx_test_remote_nonce_set(s.bob, &s.alice_n);
    rlpx_test_remote_nonce_set(s.alice, &s.bob_n);

    rlpx_ch_auth_load(s.bob, s.auth, s.authlen);
    rlpx_ch_ack_load(s.alice, s.ack, s.acklen);
    IF_ERR_EXIT(rlpx_expect_secrets(s.bob, 0, s.ack, s.acklen, s.auth,
                                    s.authlen, aes, mac, foo));
    IF_ERR_EXIT(rlpx_expect_secrets(s.alice, 1, s.auth, s.authlen, s.ack,
                                    s.acklen, aes, mac, foo));
EXIT:
    test_session_deinit(&s);
    return err;
}

//
//
//
