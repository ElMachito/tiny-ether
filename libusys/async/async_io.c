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

// Override system IO with MOCK implementation OR other IO implementation.
// Useful for test or portability.
async_io_settings g_async_io_settings_default = {
    .on_connect = async_io_default_on_connect,
    .on_accept = async_io_default_on_accept,
    .on_erro = async_io_default_on_erro,
    .on_send = async_io_default_on_send,
    .on_recv = async_io_default_on_recv,
    .tx = usys_send_to,
    .rx = usys_recv_from,
    .ready = usys_sock_ready,
    .connect = usys_connect,
    .close = usys_close
};

// Private prototypes.
void async_error(async_io* self, int);

// Public
void
async_io_init_udp(async_io* self, void* ctx, const async_io_settings* opts)
{
    async_io_init(self, ctx, opts);
    self->addr_ptr = &self->addr;
}

void
async_io_init(async_io* self, void* ctx, const async_io_settings* opts)
{
    // Zero mem
    memset(self, 0, sizeof(async_io));

    // Init state
    self->sock = -1;
    self->ctx = ctx;
    self->settings = g_async_io_settings_default;

    // override defaults with callers mock functions.
    if (opts->on_connect) self->settings.on_connect = opts->on_connect;
    if (opts->on_accept) self->settings.on_accept = opts->on_accept;
    if (opts->on_erro) self->settings.on_erro = opts->on_erro;
    if (opts->on_send) self->settings.on_send = opts->on_send;
    if (opts->on_recv) self->settings.on_recv = opts->on_recv;
    if (opts->tx) self->settings.tx = opts->tx;
    if (opts->rx) self->settings.rx = opts->rx;
    if (opts->ready) self->settings.ready = opts->ready;
    if (opts->connect) self->settings.connect = opts->connect;
    if (opts->close) self->settings.close = opts->close;
}

void
async_io_deinit(async_io* self)
{
    if (ASYNC_IO_SOCK(self)) self->settings.close(&self->sock);
    memset(self, 0, sizeof(async_io));
}

int
async_io_connect(async_io* self, const char* ip, uint32_t p)
{
    if (ASYNC_IO_SOCK(self)) self->settings.close(&self->sock);
    int ret = self->settings.connect(&self->sock, ip, p);
    if (ret < 0) {
        ASYNC_IO_SET_ERRO(self);
    } else if (ret == 0) {
        self->state |= ASYNC_IO_STATE_SEND;
    } else {
        self->state |= ASYNC_IO_STATE_RECV | ASYNC_IO_STATE_READY;
        self->settings.on_connect(self->ctx);
    }
    return ret;
}

void
async_io_close(async_io* self)
{
    ASYNC_IO_CLOSE(self);
}

void*
async_io_mem(async_io* self, uint32_t idx)
{
    return &self->b[idx];
}

void
async_io_len_set(async_io* self, uint32_t len)
{
    self->len = len;
}

uint32_t
async_io_len(async_io* self)
{
    return self->len;
}

const void*
async_io_memcpy(async_io* self, uint32_t idx, void* mem, size_t l)
{
    self->len = idx + l;
    return memcpy(&self->b[idx], mem, l);
}

int
async_io_print(async_io* self, uint32_t idx, const char* fmt, ...)
{
    int l;
    va_list ap;
    va_start(ap, fmt);
    l = vsnprintf((char*)&self->b[idx], sizeof(self->b) - idx, fmt, ap);
    if (l >= 0) self->len = l;
    va_end(ap);
    return l;
}

int
async_io_send(async_io* self)
{
    if (ASYNC_IO_SOCK(self)) {
        ASYNC_IO_SET_SEND(self);
        return 0;
    } else {
        return -1;
    }
}

int
async_io_recv(async_io* self)
{
    if (ASYNC_IO_SOCK(self)) {
        ASYNC_IO_SET_RECV(self);
        return 0;
    } else {
        return -1;
    }
}

int
async_io_poll_n(async_io** io, uint32_t n, uint32_t ms)
{
    uint32_t mask = 0;
    int reads[n], writes[n], err;
    for (uint32_t c = 0; c < n; c++) {
        reads[c] = async_io_state_recv(io[c]) ? io[c]->sock : -1;
        writes[c] = async_io_state_send(io[c]) ? io[c]->sock : -1;
    }
    err = usys_select(&mask, &mask, ms, reads, n, writes, n);
    if (mask) {
        for (uint32_t i = 0; i < n; i++) {
            if (mask & (0x01 << i)) async_io_poll(io[i]);
        }
    }
    return err;
}

int
async_io_poll(async_io* self)
{
    int c, ret = -1, end = self->len, start = self->c;
    ((void)start);
    if (!(ASYNC_IO_READY(self->state))) {
        if (ASYNC_IO_SOCK(self)) {
            ret = self->settings.ready(&self->sock);
            if (ret < 0) {
                ASYNC_IO_SET_ERRO(self);
            } else {
                ASYNC_IO_SET_READY(self);
                ASYNC_IO_SET_RECV(self);
                self->settings.on_connect(self->ctx);
            }
        } else {
        }
    } else if (ASYNC_IO_SEND(self->state)) {
        for (c = 0; c < 2; c++) {
            ret = self->settings.tx(
                &self->sock,
                &self->b[self->c],
                self->len - self->c,
                self->addr_ptr);
            if (ret >= 0) {
                if (ret + (int)self->c == end) {
                    self->settings.on_send(self->ctx, 0, self->b, self->len);
                    ASYNC_IO_SET_RECV(self); // Send complete, put into listen
                    break;
                } else if (ret == 0) {
                    ret = 0; // OK, but maybe more to send
                    break;
                } else {
                    self->c += ret;
                    ret = 0; // OK, but maybe more to send
                }
            } else {
                self->settings.on_send(self->ctx, -1, 0, 0); // IO error
                ASYNC_IO_SET_ERRO(self);
            }
        }
    } else if (ASYNC_IO_RECV(self->state)) {
        for (c = 0; c < 2; c++) {
            ret = self->settings.rx(
                &self->sock,
                &self->b[self->c],
                self->len - self->c,
                self->addr_ptr);
            if (ret >= 0) {
                if (ret + (int)self->c == end) {
                    self->settings.on_recv(self->ctx, -1, 0, 0);
                    ASYNC_IO_SET_ERRO(self);
                    break;
                } else if (ret == 0) {
                    if (c == 0) {
                        // When a readable socket returns 0 bytes on first then
                        // that means remote has disconnected.
                        ASYNC_IO_SET_ERRO(self);
                    } else {
                        self->settings.on_recv(self->ctx, 0, self->b, self->c);
                        ret = 0; // OK no more data
                    }
                    break;
                } else {
                    self->c += ret;
                    ret = 0; // OK maybe more data
                }
            } else {
                self->settings.on_recv(self->ctx, -1, 0, 0); // IO error
                ASYNC_IO_SET_ERRO(self);
            }
        }
    }
    return ret;
}

void
async_io_set_cb_recv(async_io* self, async_io_on_recv_fn fn)
{
    self->settings.on_recv = fn ? fn : async_io_default_on_recv;
}

void
async_io_set_cb_send(async_io* self, async_io_on_send_fn fn)
{
    self->settings.on_send = fn ? fn : async_io_default_on_send;
}

int
async_io_sock(async_io* self)
{
    return ASYNC_IO_SOCK(self) ? self->sock : -1;
}

int
async_io_has_sock(async_io* self)
{
    return ASYNC_IO_SOCK(self);
}

int
async_io_state_recv(async_io* self)
{
    return ASYNC_IO_READY(self->state) && ASYNC_IO_RECV(self->state);
}

int
async_io_state_send(async_io* self)
{
    return ASYNC_IO_READY(self->state) ? ASYNC_IO_SEND(self->state) : 1;
}
