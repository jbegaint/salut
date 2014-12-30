#include <stdlib.h>

#include "context.h"
#include "circbuf.h"
#include "general.h"

Context *ctx_init(int *running_ptr, int n_elt)
{
	Context *ctx = calloc(1, sizeof(*ctx));
	CircularBuffer *cb_in, *cb_out;
	size_t sz;

	/*
	 * Init circular buffers. We use an n_elt elements array, each element being
	 * a row of BUF_SIZE samples (for each NUM_CHANNELS).
	 */
	sz = NUM_CHANNELS * BUF_SIZE * sizeof(float);

	cb_in = cb_init(n_elt, sz);
	cb_out = cb_init(n_elt, sz);

	/* init context */
	ctx->running = running_ptr;
	ctx->cb_in = cb_in;
	ctx->cb_out = cb_out;

	return ctx;
}

void ctx_free(Context *ctx)
{
	/* free circular buffers */
	cb_free(ctx->cb_in);
	cb_free(ctx->cb_out);

	/* free context */
	free(ctx);
}

