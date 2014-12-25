#include <stdio.h>
#include <stdlib.h>
/* #include <math.h> */

#include "lpc.h"
#include "utils.h"

#define handle_alloc_error(x) if (!x) die("Could not allocate memory.\n")

LpcData *lpc_data_init(const size_t input_len)
{
	LpcData *lpc_data = NULL;
	LpcChunk *chunks = NULL;

	size_t n_chunks = input_len / WINDOW_SIZE;

	/* create a struct to store the lpc analysis results */
	lpc_data = calloc(1, sizeof(*lpc_data));
	handle_alloc_error(lpc_data);

	/* allocate memory for the chunks */
	chunks = calloc(n_chunks, sizeof(*chunks));
	handle_alloc_error(chunks);

	/* fill struct info */
	lpc_data->n_chunks = n_chunks;
	lpc_data->chunk_size = WINDOW_SIZE;
	lpc_data->chunks = chunks;

	return lpc_data;
}

void lpc_data_free(LpcData *lpc_data)
{
	/* free the mallocs... */
	free(lpc_data->chunks);
	free(lpc_data);
}

void lpc_detect_voiced(const float *input, LpcData *lpc_data)
{
	unsigned int i, j, c;
	float val1, val2, p, f;

	/* count the number of zero crossings in each chunks */
	for (i = 0; i < lpc_data->n_chunks; ++i) {

		/* zero crossing counter */
		c = 0;

		/* TODO: discard chunk if qualifies as (white) noise */
		for (j = 1; j < lpc_data->chunk_size; ++j) {
			val1 = input[i * lpc_data->chunk_size + j];
			val2 = input[i * lpc_data->chunk_size + j + 1];

			/* yup, it's a zero crossing */
			if ((val1 * val2) < 0) {
				c++;
			}
		}

		/* TODO: skip these steps by using a threshold directly based on the
		 * number of zero crossings. */

		/* compute period */
		p = 2 * lpc_data->chunk_size / c;

		/* compute frequency */
		f = 1 / (2 * p);

		/* set as voiced if criterion is fullfilled */
		if (f > F_THRESHOLD) {
			lpc_data->chunks[i].pitch = 1;
		}
	}
}

void lpc_analysis(LpcChunk *lpc_chunk)
{
	UNUSED(lpc_chunk);
}

LpcData *lpc_encode(const float *input, const size_t input_len)
{
	LpcData *lpc_data;
	LpcChunk *lpc_chunk;
	unsigned int i;

	/* create a struct for the results */
	lpc_data = lpc_data_init(input_len);

	/* pre emphasis filter */
	/* TODO */

	/* detect voiced/ non-voiced sound */
	lpc_detect_voiced(input, lpc_data);

	/* use autocorrelation to compute the pitch (because ADMF sucks) */
	for (i = 0; i < lpc_data->n_chunks; ++i) {
		lpc_chunk = &lpc_data->chunks[i];

		/* i.e.: non voiced */
		if (lpc_chunk->pitch == 0)
			continue;

		/* apodization */
		/* autocorr */
		/* discard mirror parts */
		/* findpeaks */
		/* extract pitch (2nd max) */
	}

	/* compute lpc coefficients */
	for (i = 0; i < lpc_data->n_chunks; ++i) {
		lpc_chunk = &lpc_data->chunks[i];

		/* compute lpc */
		lpc_analysis(lpc_chunk);
	}

	return lpc_data;
}
