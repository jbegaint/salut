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

#include <arpa/inet.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "udp.h"
#include "utils.h"

void init_peer_addr(struct sockaddr_in *peer_addr, int host)
{
	CLEAR(*peer_addr);

	peer_addr->sin_family = AF_INET;
	peer_addr->sin_port = htons(DFLT_PORT);
	peer_addr->sin_addr.s_addr = htonl(host);
}

int send_msg(int fd, struct sockaddr_in *addr_ptr, const void *msg, size_t sz)
{
	int sn;
	socklen_t socklen = sizeof(struct sockaddr_in);

	sn = sendto(fd, msg, sz, 0, (struct sockaddr *) addr_ptr, socklen);

	if (sn == -1)
		errno_die();

	return sn;
}

int recv_msg(int fd, struct sockaddr_in *addr_ptr, void *buf, size_t buf_len)
{
	int rc;
	socklen_t socklen = sizeof(struct sockaddr_in);

	rc = recvfrom(fd, buf, buf_len, 0, (struct sockaddr *) addr_ptr, &socklen);

	if ((rc == -1) && (errno != EAGAIN))
		errno_die();

	return rc;
}
