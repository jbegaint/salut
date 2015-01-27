/*
 * Copyright (c) 2014 - 2015 Jean Bégaint
 *
 * This file is part of salut.
 *
 * salut is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * salut is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with salut.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _UDP_H_
#define _UDP_H_

#define DFLT_PORT 1337

void init_peer_addr(struct sockaddr_in *peer_addr, int host);

int send_msg(int fd, struct sockaddr_in *addr_ptr, const void *msg, size_t sz);
int recv_msg(int fd, struct sockaddr_in *addr_ptr, void *buf, size_t buf_len);

#endif
