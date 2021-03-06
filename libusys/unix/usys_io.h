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

/**
 * @file usys_io.h
 *
 * @brief Simple wrapper around OS primitives to better support non standard
 * enviorments.
 */
#ifndef USYS_IO_H_
#define USYS_IO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "usys_config.h"

typedef int usys_socket_fd;
typedef int usys_file_fd;
typedef unsigned char byte;
typedef struct
{
    uint32_t ip;
    uint32_t port;
} usys_sockaddr;

// File sys call abstraction layer
usys_file_fd usys_file_open(const char* path);
int usys_file_write(usys_file_fd* fd, int offset, const char* data, uint32_t l);
int usys_file_read(usys_file_fd* fd, int offset, char* data, uint32_t l);
int usys_file_close(usys_file_fd* fd);

// Networking sys call abstraction layer
typedef int (*usys_io_send_fn)(usys_socket_fd*, const byte*, uint32_t);
typedef int (*usys_io_send_to_fn)(
    usys_socket_fd*,
    const byte*,
    uint32_t,
    usys_sockaddr*);
typedef int (*usys_io_recv_fn)(usys_socket_fd*, byte*, uint32_t);
typedef int (
    *usys_io_recv_from_fn)(usys_socket_fd*, byte*, uint32_t, usys_sockaddr*);
typedef int (*usys_io_connect_fn)(usys_socket_fd*, const char*, int);
typedef int (*usys_io_ready_fn)(usys_socket_fd*);
typedef void (*usys_io_close_fn)(usys_socket_fd*);
int usys_connect(usys_socket_fd* fd, const char* host, int port);
int usys_listen_udp(usys_socket_fd* sock_p, int port);
int usys_send_fd(usys_socket_fd fd, const byte* b, uint32_t len);
int usys_send_to_fd(usys_socket_fd, const byte*, uint32_t, usys_sockaddr*);
int usys_recv_fd(int sockfd, byte* b, size_t len);
int usys_recv_from_fd(int sockfd, byte* b, size_t len, usys_sockaddr*);
void usys_close(usys_socket_fd* fd);
void usys_close_fd(usys_socket_fd s);
int usys_sock_error(usys_socket_fd* fd);
int usys_sock_ready(usys_socket_fd* fd);
uint32_t usys_select(
    uint32_t* rmask,
    uint32_t* wmask,
    int time,
    int* reads,
    int nreads,
    int* writes,
    int nwrites);
static inline int
usys_send(usys_socket_fd* fd, const byte* b, uint32_t len)
{
    return usys_send_fd(*(usys_socket_fd*)fd, b, len);
}

static inline int
usys_recv(usys_socket_fd* fd, byte* b, uint32_t len)
{
    return usys_recv_fd(*(usys_socket_fd*)fd, b, len);
}

static inline int
usys_send_to(
    usys_socket_fd* fd,
    const byte* b,
    uint32_t len,
    usys_sockaddr* addr)
{
    return usys_send_to_fd(*(usys_socket_fd*)fd, b, len, addr);
}

static inline int
usys_recv_from(usys_socket_fd* fd, byte* b, uint32_t len, usys_sockaddr* addr)
{
    return usys_recv_from_fd(*(usys_socket_fd*)fd, b, len, addr);
}

#ifdef __cplusplus
}
#endif
#endif
