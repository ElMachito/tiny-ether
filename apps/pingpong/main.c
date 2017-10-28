#include "ueth.h"
#include "usys_log.h"
#include "usys_signals.h"
#include "usys_time.h"

#define SKEY "5e173f6ac3c669587538e7727cf19b782a4f2fda07c1eaa662c593e5e85e3051"
#define REMOTE                                                                 \
    "ca634cae0d49acb401d8a4c6b6fe8c55b70d115bf400769cc1400f3258cd31387574077f" \
    "301b421bc84df7266c44e9e6d569fc56be00812904767bf5ccd1fc7f"

const char* g_test_enode = "enode://" REMOTE "@127.0.0.1:30303";

ueth_config config = { //
    .p2p_private_key = SKEY,
    .p2p_enable = 1,
    .udp = 22332
};

int main(int argc, char* argv[]);

int
main(int argc, char* argv[])
{
    ((void)argc);
    ((void)argv);
    ueth_context eth;

    // Log message
    usys_log_note("Running ping pong demo");

    // Install interrupt control
    usys_install_signal_handlers();

    // start
    ueth_init(&eth, &config);
    ueth_start(&eth, 1, g_test_enode);

    while (usys_running()) {
        // Poll io
        usys_msleep(200);
        ueth_poll(&eth);
    }

    // Notify remotes of shutdown and clean
    ueth_stop(&eth);
    ueth_deinit(&eth);
    return 0;
}
