#ifndef _UDP_H_
#define _UDP_H_

#define DFLT_PORT 1337

void init_peer_addr(struct sockaddr_in *peer_addr, int host);

int send_msg(int fd, struct sockaddr_in *addr_ptr, const void *msg, size_t sz);
int recv_msg(int fd, struct sockaddr_in *addr_ptr, void *buf, size_t buf_len);

#endif
