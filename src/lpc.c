#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <liquid/liquid.h>

#include "general.h"
#include "lpc.h"
#include "utils.h"

#define handle_alloc_error(x) if (!x) die("Could not allocate memory.\n")

int get_pitch_by_amdf(float *input, const size_t len)
{
	/*
	 * Min and max pitch *in sample*. We use 50Hz and 300Hz as arbitrary pitch
	 * limits.
	 */
	const unsigned int min_pitch = SAMPLE_RATE / 300;
	const unsigned int max_pitch = SAMPLE_RATE / 50;
	int pitch = 0;
	float max_d = 0;
	unsigned int i, j;
	float *d = NULL;

	d = calloc(max_pitch - min_pitch, sizeof(*d));
	handle_alloc_error(d);

	/* compute the difference signals */
	for (i = min_pitch; i < max_pitch; ++i) {

		for (j = i; j < len; ++j) {
			d[i - min_pitch] += fabs(input[i] - input[j - i]);
		}

		/* mean */
		d[i - min_pitch] /= len - i + 1;
	}

	/* determine optimal pitch */
	for (i = 0; i < max_pitch - min_pitch; ++i) {
		if (max_d < d[i]) {
			max_d = d[i];
			pitch = i;
		}
	}

	/* correct the offset */
	pitch += min_pitch;

	/* free the mallocs */
	free(d);

	return pitch;
}

void lpc_detect_voiced(float *input, LpcChunk *lpc_chunk)
{
	unsigned int j, c;
	float val1, val2, p, f;

	/* zero crossing counter */
	c = 1;

	/* TODO: discard chunk if chunk qualifies as (white) noise */
	for (j = 1; j < CHUNK_SIZE; ++j) {
		val1 = input[CHUNK_SIZE + j];
		val2 = input[CHUNK_SIZE + j + 1];

		/* yup, it's a zero crossing */
		if ((val1 * val2) < 0) {
			c++;
		}
	}

	/* TODO: skip these steps by using a threshold directly based on the
	 * number of zero crossings (converted from the max frequency). */

	/* compute period */
	p = 2 * CHUNK_SIZE / c;

	/* compute frequency */
	f = 1 / (2 * p);

	/* set as voiced if criterion is fullfilled */
	if (f > F_THRESHOLD) {
		lpc_chunk->pitch = 1;
	}
}

LpcChunk lpc_encode(float *input)
{
	/* pre emphasis filter */
	/* TODO */
	LpcChunk lpc_chunk;

	/* detect voiced/ non-voiced sound */
	lpc_detect_voiced(input, &lpc_chunk);

	/* TODO: use autocorrelation to compute the pitch (?) */

	/* as AMDF is simpler to implement, let's use it for now */

	/* skip step for non-voiced sounds */
	if (lpc_chunk.pitch == 1) {
		lpc_chunk.pitch = get_pitch_by_amdf(input, CHUNK_SIZE);
	}

	/* useless for now (TODO ?) */
	float g[N_COEFFS];

	/* compute lpc */
	my_liquid_lpc(input, CHUNK_SIZE, N_COEFFS, lpc_chunk.coefficients, g);

	return lpc_chunk;
}

void lpc_decode(LpcChunk *lpc_chunk, float *output)
{
	unsigned int i;
	float a_lpc[N_COEFFS];
	float b_lpc[N_COEFFS];
	float excitation[CHUNK_SIZE];

	float *coeffs = lpc_chunk->coefficients;

	/* compute the excitation */
	if (lpc_chunk->pitch > 0) {
		printf("%d\n", lpc_chunk->pitch);

		/* if voiced, generate an impulsion train */
		excitation[0] = 1;
		for (i = 1; i < CHUNK_SIZE; ++i) {
			excitation[i] = ((i % lpc_chunk->pitch) == 0) ? 1 : 0;
		}
	}
	else {
		/* generate a white noise */
		for (i = 0; i < CHUNK_SIZE; ++i) {
			excitation[i] = randnf();
		}
	}

	/* init filter coefficients */
	for (i = 0; i < N_COEFFS; ++i) {
		a_lpc[i] = (i == 0) ? 1 : 0;
		/* or MINUS ? */
		b_lpc[i] = (i == 0) ? 0 : coeffs[i];
	}

	/* filter the excitation */
	iirfilt_rrrf f = iirfilt_rrrf_create(b_lpc, N_COEFFS, a_lpc, N_COEFFS);

	for (i = 0; i < CHUNK_SIZE; ++i) {
		iirfilt_rrrf_execute(f, excitation[i], &output[i]);
	}

	iirfilt_rrrf_destroy(f);
}
