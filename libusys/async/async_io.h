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

#ifndef ASYNC_ASYNC_IO_H_
#define ASYNC_ASYNC_IO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "usys_config.h"
#include "usys_io.h"

#define ASYNC_IO_STATE_READY (0x01 << 0)
#define ASYNC_IO_STATE_ERRO (0x01 << 1)
#define ASYNC_IO_STATE_SEND (0x01 << 2)
#define ASYNC_IO_STATE_RECV (0x01 << 3)

#define ASYNC_IO_READY(x) ((x) & (ASYNC_IO_STATE_READY))
#define ASYNC_IO_SEND(x) ((x) & (ASYNC_IO_STATE_SEND))
#define ASYNC_IO_RECV(x) ((x) & (ASYNC_IO_STATE_RECV))
#define ASYNC_IO_ERRO(x) ((x) & (ASYNC_IO_STATE_ERRO))
#define ASYNC_IO_SOCK(x) ((x)->sock >= 0)

#define ASYNC_IO_SET_SEND(x)                                                   \
    do {                                                                       \
        (x)->state |= ASYNC_IO_STATE_SEND;                                     \
        (x)->state &= (~(ASYNC_IO_STATE_RECV));                                \
        (x)->c = 0;                                                            \
    } while (0)

#define ASYNC_IO_SET_RECV(x)                                                   \
    do {                                                                       \
        (x)->state |= ASYNC_IO_STATE_RECV;                                     \
        (x)->state &= (~(ASYNC_IO_STATE_SEND));                                \
        (x)->c = 0;                                                            \
        (x)->len = 1200;                                                       \
    } while (0)

#define ASYNC_IO_SET_ERRO(x)                                                   \
    do {                                                                       \
        (x)->settings.on_erro((x)->ctx);                                       \
        (x)->state = 0;                                                        \
        (x)->c = 0;                                                            \
        (x)->len = 0;                                                          \
        if (ASYNC_IO_SOCK((x))) usys_close(&(x)->sock);                        \
    } while (0)

#define ASYNC_IO_SET_READY(x)                                                  \
    do {                                                                       \
        (x)->state |= ASYNC_IO_STATE_READY;                                    \
        (x)->c = 0;                                                            \
        (x)->len = 0;                                                          \
    } while (0)

#define ASYNC_IO_CLOSE(x)                                                      \
    do {                                                                       \
        (x)->state = 0;                                                        \
        (x)->c = 0;                                                            \
        (x)->len = 0;                                                          \
        if (ASYNC_IO_SOCK((x))) usys_close(&(x)->sock);                        \
    } while (0)

typedef int (*async_io_on_connect_fn)(void*);
typedef int (*async_io_on_accept_fn)(void*);
typedef int (*async_io_on_erro_fn)(void*);
typedef int (*async_io_on_send_fn)(void*, int err, const uint8_t* b, uint32_t);
typedef int (*async_io_on_recv_fn)(void*, int err, uint8_t* b, uint32_t);

typedef struct async_io_settings
{
    async_io_on_connect_fn on_connect;
    async_io_on_accept_fn on_accept;
    async_io_on_erro_fn on_erro;
    async_io_on_send_fn on_send;
    async_io_on_recv_fn on_recv;
    usys_io_send_to_fn tx;
    usys_io_recv_from_fn rx;
    usys_io_ready_fn ready;
    usys_io_connect_fn connect;
    usys_io_close_fn close;
} async_io_settings;

typedef struct
{
    usys_socket_fd sock;
    uint32_t state;
    async_io_settings settings;
    void* ctx;
    usys_sockaddr addr;
    usys_sockaddr* addr_ptr;
    uint32_t c, len;
    uint8_t b[1200];
} async_io;

void async_io_install(usys_io_send_fn s, usys_io_recv_fn r);
void async_io_init_udp(async_io*, void*, const async_io_settings*);
void async_io_init(async_io*, void*, const async_io_settings*);
void async_io_deinit(async_io* self);
int async_io_connect(async_io* async, const char* ip, uint32_t p);
void async_io_close(async_io* self);
void* async_io_mem(async_io* self, uint32_t idx);
void async_io_len_set(async_io* self, uint32_t len);
uint32_t async_io_len(async_io* self);
const void* async_io_memcpy(async_io* self, uint32_t idx, void* mem, size_t l);
int async_io_print(async_io* self, uint32_t, const char* fmt, ...);
int async_io_send(async_io*);
int async_io_recv(async_io*);
int async_io_poll_n(async_io** io, uint32_t n, uint32_t ms);
int async_io_poll(async_io*);
void async_io_set_cb_recv(async_io* self, async_io_on_recv_fn fn);
void async_io_set_cb_send(async_io* self, async_io_on_send_fn fn);
int async_io_sock(async_io* self);
int async_io_has_sock(async_io* self);
int async_io_state_recv(async_io* self);
int async_io_state_send(async_io* self);

static inline int
async_io_default_on_connect(void* ctx)
{
    ((void)ctx);
    return 0;
}

static inline int
async_io_default_on_accept(void* ctx)
{
    ((void)ctx);
    return 0;
}
static inline int

async_io_default_on_erro(void* ctx)
{
    ((void)ctx);
    return 0;
}

static inline int
async_io_default_on_send(void* ctx, int err, const uint8_t* b, uint32_t l)
{
    ((void)ctx);
    ((void)err);
    ((void)b);
    ((void)l);
    return 0;
}

static inline int
async_io_default_on_recv(void* ctx, int err, uint8_t* b, uint32_t l)
{
    ((void)ctx);
    ((void)err);
    ((void)b);
    ((void)l);
    return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
