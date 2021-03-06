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

#include "async_io.h"

// 56 byte test vector
char* g_lorem = "Lorem ipsum dolor sit amet, consectetur adipisicing elit";

// Mock overrides
int io_mock_ready(usys_socket_fd*);
int io_mock_connect(usys_socket_fd* fd, const char* host, int port);
void io_mock_close(usys_socket_fd* fd);
int
io_mock_send_all(usys_socket_fd* fd, const byte* b, uint32_t l, usys_sockaddr*);
int
io_mock_send_one(usys_socket_fd* fd, const byte* b, uint32_t l, usys_sockaddr*);
int
io_mock_send_min(usys_socket_fd* fd, const byte* b, uint32_t l, usys_sockaddr*);
int io_mock_recv(usys_socket_fd* fd, byte* b, uint32_t l, usys_sockaddr*);

// Callbacks from IO
int io_on_connect(void* ctx);
int io_on_accept(void* ctx);
int io_on_erro(void* ctx);
int io_on_send(void* ctx, int err, const uint8_t* b, uint32_t l);
int io_on_recv(void* ctx, int err, uint8_t* b, uint32_t l);

typedef struct
{
    int n;
    async_io_settings* settings;
} io_test_settings;

async_io_settings g_io_settings_all = {.on_connect = io_on_connect,
                                       .on_accept = io_on_accept,
                                       .on_erro = io_on_erro,
                                       .on_send = io_on_send,
                                       .on_recv = io_on_recv,
                                       .ready = io_mock_ready,
                                       .connect = io_mock_connect,
                                       .close = io_mock_close,
                                       .tx = io_mock_send_all,
                                       .rx = io_mock_recv };
async_io_settings g_io_settings_one = {.on_connect = io_on_connect,
                                       .on_accept = io_on_accept,
                                       .on_erro = io_on_erro,
                                       .on_send = io_on_send,
                                       .on_recv = io_on_recv,
                                       .ready = io_mock_ready,
                                       .connect = io_mock_connect,
                                       .close = io_mock_close,
                                       .tx = io_mock_send_one,
                                       .rx = io_mock_recv };
async_io_settings g_io_settings_min = {.on_connect = io_on_connect,
                                       .on_accept = io_on_accept,
                                       .on_erro = io_on_erro,
                                       .on_send = io_on_send,
                                       .on_recv = io_on_recv,
                                       .ready = io_mock_ready,
                                       .connect = io_mock_connect,
                                       .close = io_mock_close,
                                       .tx = io_mock_send_min,
                                       .rx = io_mock_recv };

int
main(int argc, char* argv[])
{
    ((void)argc);
    ((void)argv);

    // Stack
    int err = 0, i, c;
    async_io io;
    io_test_settings settings[] = {
        {.n = 1, .settings = &g_io_settings_all },  // Test one shot send/recv
        {.n = 28, .settings = &g_io_settings_one }, // Test busy io
        {.n = 10, .settings = &g_io_settings_min }  // Test less busy io
    };

    // Run test
    for (i = 0; i < 3; i++) {
        err = -1;
        async_io_init(&io, &err, settings[i].settings);

        // Mock peer
        async_io_connect(&io, "thhpt", 8080);
        async_io_poll(&io);

        // Test transmit (prepare send buffer and send)
        async_io_print(&io, 0, "%s", g_lorem);
        async_io_send(&io);
        for (c = 0; c < settings[i].n; c++) async_io_poll(&io);

        // Did send callback signal Send complete?
        if (err) break;

        // Test receive.
        async_io_deinit(&io);
    }
    return err;
}

int
io_mock_connect(usys_socket_fd* fd, const char* host, int port)
{
    ((void)host);
    ((void)port);
    static int sock = 0;     // 0 is a valid socket.
    if (*fd >= 0) return -1; // err already connect
    *fd = sock++;
    return 0;
}

int
io_mock_ready(usys_socket_fd* fd)
{
    return *fd;
}

void
io_mock_close(usys_socket_fd* fd)
{
    *fd = -1;
}

int
io_mock_send_all(
    usys_socket_fd* fd,
    const byte* b,
    uint32_t l,
    usys_sockaddr* addr)
{
    ((void)fd);
    ((void)b);
    ((void)addr);
    return l;
}

int
io_mock_send_one(
    usys_socket_fd* fd,
    const byte* b,
    uint32_t l,
    usys_sockaddr* addr)
{
    ((void)fd);
    ((void)b);
    ((void)addr);
    return l ? 1 : 0;
}

int
io_mock_send_min(
    usys_socket_fd* fd,
    const byte* b,
    uint32_t l,
    usys_sockaddr* addr)
{
    ((void)fd);
    ((void)b);
    ((void)addr);
    return (l < 3) ? l : 3;
}

int
io_mock_recv(usys_socket_fd* fd, byte* b, uint32_t l, usys_sockaddr* addr)
{
    ((void)fd);
    ((void)b);
    ((void)l);
    ((void)addr);
    return 0; // TODO - need test vectors.
}

int
io_on_connect(void* ctx)
{
    ((void)ctx);
    return 0;
}

int
io_on_accept(void* ctx)
{
    ((void)ctx);
    return 0;
}

int
io_on_erro(void* ctx)
{
    ((void)ctx);
    return 0;
}

int
io_on_send(void* ctx, int err, const uint8_t* b, uint32_t l)
{
    ((void)err);
    ((void)b);
    ((void)l);
    *(int*)ctx = 0;
    return 0;
}

int
io_on_recv(void* ctx, int err, uint8_t* b, uint32_t l)
{
    ((void)ctx);
    ((void)err);
    ((void)b);
    ((void)l);
    return 0;
}
