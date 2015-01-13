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
