#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "general.h"
#include "lpc.h"
#include "vorbis_lpc.h"
#include "utils.h"

#define handle_alloc_error(x) if (!x) die("Could not allocate memory.\n")

/* void hanning(const float *input, const size_t len, float *output) */
/* { */
/* 	unsigned int i; */
/* 	float coef; */

/* 	for (i = 0; i < len; ++i) { */
/* 		coef = 0.5 - 0.5 * cos(2 * M_PI * i / len); */
/* 		*output++ = *input++ * coef; */
/* 	} */
/* } */

float get_pitch_by_amdf(float *input, const size_t len)
{
	/*
	 * Min and max pitch *in sample*. We use 50Hz and 300Hz as arbitrary pitch
	 * limits.
	 */
	const unsigned int min_pitch = SAMPLE_RATE / 300;
	const unsigned int max_pitch = SAMPLE_RATE / 50;
	float pitch = 0;
	unsigned int i, j;
	float *d = NULL;

	d = calloc(max_pitch - min_pitch, sizeof(*d));
	handle_alloc_error(d);

	/* compute the difference signals */
	for (i = min_pitch; i < max_pitch; ++i) {

		for (j = i; j < len; ++j) {
			/* d[i] += fabs(input[i] - input[i - j]); */
			d[i - min_pitch] += fabs(input[i] - input[j - i]);
		}

		/* mean */
		d[i - min_pitch] /= len - i + 1;
	}

	/* determine optimal pitch */
	for (i = 0; i < max_pitch - min_pitch; ++i) {
		if (pitch < d[i])
			pitch = d[i];
	}

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
	if (lpc_chunk.pitch == 0) {

	}
	else {
		lpc_chunk.pitch = get_pitch_by_amdf(input, CHUNK_SIZE);
	}

	/* Compute LPC thx to libvorbis function. TODO: use the error ? */
	vorbis_lpc_from_data(input, lpc_chunk.coefficients,
			CHUNK_SIZE, N_COEFFS);

	return lpc_chunk;
}

void lpc_decode(LpcChunk *lpc_chunk, float *output)
{
	/* compute excitation */

	if (lpc_chunk->pitch > 0) {
		/* voiced sound, use a pulse impulsion train */

	}
	else {
		/* non voiced sound, use a white noise */
	}

	printf("chunk pitch: %f\n", lpc_chunk->pitch);

	/* lpc decode */
	vorbis_lpc_predict(lpc_chunk->coefficients, NULL, N_COEFFS, output,
			CHUNK_SIZE);
}
