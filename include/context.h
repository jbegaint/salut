#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "circbuf.h"

typedef struct {
	int *running;
	int *socket_fd;

	CircularBuffer *cb_in, *cb_out;

	struct sockaddr_in *myaddr, *peeraddr;
} Context;


Context *ctx_init(int *running_ptr, int n_elt);
void ctx_free(Context *ctx);

#endif
