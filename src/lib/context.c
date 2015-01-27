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

