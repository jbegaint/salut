#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
	const unsigned int min_pitch = SAMPLE_RATE / MAX_FREQ;
	const unsigned int max_pitch = SAMPLE_RATE / MIN_FREQ;

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

	/* free the malloc */
	free(d);

	return pitch;
}

void lpc_detect_voiced(float *input, LpcChunk *lpc_chunk)
{
	unsigned int i, j, c;
	float val1, val2, p, f;

	/* zero crossing counter */
	c = 1;

	/* compute chunk energy */
	float e = 0, val;

	for (i = 0; i < CHUNK_SIZE; ++i) {
		val = fabs(input[i] * input[i]);
		e = max(e, val);
	}

	/* FIXME ? */
	if (e < 0.04) {
		/* chunk qualifies as noise, let's discard it */
		lpc_chunk->pitch = -1;
		return;
	}

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
	/* FIXME */
	if (f > F_THRESHOLD) {
		lpc_chunk->pitch = 1;
	}
}

void lpc_pre_emphasis_filter(float *input, float *output)
{
	unsigned int i;

	float a[2]= {1.f, 1.f};
	float b[2] = {1.f, -0.9f};

	iirfilt_rrrf f = iirfilt_rrrf_create(b, 2, a, 2);

	for (i = 0; i < CHUNK_SIZE; ++i) {
		iirfilt_rrrf_execute(f, input[i], &output[i]);
	}

	iirfilt_rrrf_destroy(f);
}

void lpc_de_emphasis_filter(float *input, float *output)
{
	unsigned int i;

	float a[2] = {1.f, -0.9f};
	float b[2]= {1.f, 1.f};

	iirfilt_rrrf f = iirfilt_rrrf_create(b, 2, a, 2);

	for (i = 0; i < CHUNK_SIZE; ++i) {
		iirfilt_rrrf_execute(f, input[i], &output[i]);
	}

	iirfilt_rrrf_destroy(f);
}

LpcChunk lpc_encode(float *input)
{
	LpcChunk lpc_chunk;
	float data[CHUNK_SIZE];

	/* zero struct */
	memset(&lpc_chunk, 0, sizeof(lpc_chunk));

	/* detect voiced/non-voiced sound */
	lpc_detect_voiced(input, &lpc_chunk);

	/*
	 * TODO: use autocorrelation to compute the pitch (?), as AMDF is simpler to
	 * implement, let's use it for now.
	 */

	/* skip step for non-voiced sounds */
	if (lpc_chunk.pitch == 1) {
		lpc_chunk.pitch = get_pitch_by_amdf(data, CHUNK_SIZE);
	}
	else if (lpc_chunk.pitch == -1) {
		return lpc_chunk;
	}

	/* prediction error variances, useless for now (TODO ?) */
	float g[N_COEFFS];

	/* compute lpc */

	/* pre emphasis filter */
	/* lpc_pre_emphasis_filter(input, data); */
	memcpy(data, input, CHUNK_SIZE * sizeof(float));

	my_liquid_lpc(data, CHUNK_SIZE, N_COEFFS - 1, lpc_chunk.coefficients, g);

	return lpc_chunk;
}

void lpc_decode(LpcChunk *lpc_chunk, float *output)
{
	unsigned int i;
	float excitation[CHUNK_SIZE];

	const float *coeffs = lpc_chunk->coefficients;
	const int pitch = lpc_chunk->pitch;

	/* compute the excitation */
	if (pitch > 0) {
		/* if voiced, generate an impulsion train */
		for (i = 0; i < CHUNK_SIZE; ++i) {
			excitation[i] = ((i % pitch) == 0) ? sqrt(pitch) : 0;
		}
	}
	else if (pitch == 0) {
		/* generate a white noise */
		for (i = 0; i < CHUNK_SIZE; ++i) {
			/* uniform distribution (similar to matlab's rand()) */
			excitation[i] = (float) rand() / (float) RAND_MAX;
		}
	}
	else {
		/* hello darkness my old friend... */
		memset(output, SAMPLE_SILENCE, sizeof(excitation));

		/* all done */
		return;
	}

	/* init filter coefficients */
	float b_lpc[N_COEFFS];
	float *a_lpc = (float *) coeffs;

	memset(&b_lpc, 0, sizeof(b_lpc));
	b_lpc[0] = 1;

	/* filter the excitation */
	/* liquid dsp: iirfilt_rrrf loopfilter = iirfilt_rrrf_create(b,3,a,3); */
	/* matlab : filter(1, coeffs, ...), y = filter(b,a,x) */

	iirfilt_rrrf f = iirfilt_rrrf_create(b_lpc, N_COEFFS, a_lpc, N_COEFFS);

	for (i = 0; i < CHUNK_SIZE; ++i) {
		iirfilt_rrrf_execute(f, excitation[i], &output[i]);
	}

	iirfilt_rrrf_destroy(f);

	/* de-emphasis filter */
	/* lpc_de_emphasis_filter(output, output); */
}
