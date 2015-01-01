#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <liquid/liquid.h>

#include "general.h"
#include "lpc.h"
#include "utils.h"

#define handle_alloc_error(x) if (!x) die("Could not allocate memory.\n")

#define fprintf(x,...) (void)(x)

void hanning(const float *input, const size_t len, float *output)
{
	unsigned int i;
	float coef;

	for (i = 0; i < len; ++i) {
		coef = 0.5 - 0.5 * cos(2.f * M_PI * i / len);
		*output++ = *input++ * coef;
	}
}

int get_pitch_by_autocorr(float *input, const size_t len)
{
	int pitch = -1;
	unsigned int i, k;
	float sum, old_sum, thresh;

	int state = 1;

	for ( k = 0; k < len; ++k) {
		sum += input[k] * input[k];
	}
	thresh = 0.2f * sum;

	for (i = 1; i < len; ++i) {
		old_sum = sum;
		sum = 0;

		for (k = 0; k < len - i; ++k)
			sum += input[k] * input[k + i];

		/* positive slope detected */
		if ((state == 1) && (sum > thresh) && (sum - old_sum) > 0) {
			state = 2;
		}

		/* negative slope beginning, we found the second max */
		if ((state == 2) && (sum - old_sum) < 0) {
			pitch = i;
			break;
		}
	}

	return pitch;
}

int get_pitch(float *input, const size_t len)
{
	return get_pitch_by_autocorr(input, len);
}

void lpc_pre_emphasis_filter(float *input, float *output)
{
	unsigned int i;

	float a[2]= {1.f, 1.f};
	float b[2] = {1.f, -0.9f};

	iirfilt_rrrf f = iirfilt_rrrf_create(b, 2, a, 2);

	for (i = 0; i < CHUNK_SIZE; ++i)
		iirfilt_rrrf_execute(f, input[i], &output[i]);

	iirfilt_rrrf_destroy(f);
}

void lpc_de_emphasis_filter(float *input, float *output)
{
	unsigned int i;

	float a[2] = {1.f, -0.9f};
	float b[2]= {1.f, 1.f};

	iirfilt_rrrf f = iirfilt_rrrf_create(b, 2, a, 2);

	for (i = 0; i < CHUNK_SIZE; ++i)
		iirfilt_rrrf_execute(f, input[i], &output[i]);

	iirfilt_rrrf_destroy(f);
}

LpcChunk lpc_encode(float *input)
{
	LpcChunk lpc_chunk;
	float data[CHUNK_SIZE];

	/* zero struct */
	CLEAR(lpc_chunk);

	/* compute chunk energy */
	float e = 0, val;

	for (int i = 0; i < CHUNK_SIZE; ++i) {
		val = fabs(input[i] * input[i]);
		e = MAX(e, val);
	}

	/* heuristic value (FIXME ?) */
	/* float thresh = 0.2f * 256 / (float) CHUNK_SIZE */
	const float thresh = 0.4f;

	if (e < thresh) {
		/* background noise */
		lpc_chunk.pitch = -1;
		return lpc_chunk;
	}

	/* hanning apodization */
	hanning(input, CHUNK_SIZE, input);

	/* pre emphasis filter */
	lpc_pre_emphasis_filter(input, data);

	/* compute pitch (autocorr FTW) */
	const int pitch = get_pitch(input, CHUNK_SIZE);

	if (pitch == -1) {
		/* background noise, discard */
		lpc_chunk.pitch = -1;
	}
	else if (pitch < MIN_PITCH) {
		/* non voiced */
		lpc_chunk.pitch = 0;
	}
	else if (pitch > MAX_PITCH) {
		lpc_chunk.pitch = -1;
	}
	else {
		/* voiced */
		lpc_chunk.pitch = pitch;
	}

	/*
	 * Compute the LPC coefficients. The prediction error variances are stored
	 * in g. They are useless for us, but liquid-dsp segfaults if NULL is
	 * passed.
	 */

	float g[N_COEFFS];

	my_liquid_lpc(data, CHUNK_SIZE, N_COEFFS - 1, lpc_chunk.coefficients, g);

	return lpc_chunk;
}

void lpc_decode(LpcChunk *lpc_chunk, float *output)
{
	unsigned int i;
	float excitation[CHUNK_SIZE];
	float data[CHUNK_SIZE];

	const float *coeffs = lpc_chunk->coefficients;
	const int pitch = lpc_chunk->pitch;

	/* hello darkness my old friend... */
	memset(output, SAMPLE_SILENCE, sizeof(excitation));

	/* compute the excitation */
	if (pitch > 0) {
		fprintf(stderr, "\r [x] Voiced  [ ] Unvoiced  [ ] Background Noise.");

		/* if voiced, generate an impulsion train */
		for (i = 0; i < CHUNK_SIZE; ++i) {

			excitation[i] = ((i % pitch) == 0) ? sqrt(pitch) : 0;

			/* limit saturation */
			/* excitation[i] *= 0.01f; */
			excitation[i] *= 0.005f;
		}
	}
	else if (pitch == 0) {
		fprintf(stderr, "\r [ ] Voiced  [x] Unvoiced  [ ] Background Noise.");

		/* generate a white noise */
		for (i = 0; i < CHUNK_SIZE; ++i) {

			/*
			 * Uniform distribution (similar to matlab's rand()). Generate
			 * random values between -1 and 1 (*[max-min]+min).
			 */

			excitation[i] = ((float) rand() / (float) RAND_MAX) * 2 - 1;

			/* limit saturation */
			/* excitation[i] *= 0.01f; */
			excitation[i] *= 0.005f;
		}
	}
	else {
		fprintf(stderr, "\r [ ] Voiced  [ ] Unvoiced  [x] Background Noise.");

		for (i = 0; i < CHUNK_SIZE; ++i) {
			/*
			 * Uniform distribution (similar to matlab's rand()). Generate
			 * random values between -1 and 1 (*[max-min]+min).
			 */
			output[i] = ((float) rand() / (float) RAND_MAX) * 2 - 1;

			/* limit saturation */
			output[i] *= 0.002f;

			/* (TODO: store some background noise in memory) */
		}
		return;
	}

	/* init filter coefficients */
	float a_lpc[N_COEFFS];
	float b_lpc[N_COEFFS];

	CLEAR(b_lpc);
	b_lpc[0] = 1;
	memcpy(a_lpc, coeffs, sizeof(a_lpc));

	/* BWE method */
	float y = 0.95;
	int stable;
	int c = 0;

	/* check if the filter is stable, ie: poles are within the unit circle */
	do {
		stable = 1;
		for (i = 0; i < N_COEFFS; ++i) {
			a_lpc[i] = (i == 0) ? 1 : y * a_lpc[i];

			if (fabs(a_lpc[i]) > 1)
				stable = 0;
		}
		c++;
	} while ((!stable) && (c < 20));

	if (c == 20) {
		/* discard these coefficients */
		memset(a_lpc, 0, sizeof(a_lpc));
	}

	/* filter the excitation */
	iirfilt_rrrf f = iirfilt_rrrf_create(b_lpc, N_COEFFS, a_lpc, N_COEFFS);

	for (i = 0; i < CHUNK_SIZE; ++i)
		iirfilt_rrrf_execute(f, excitation[i], &data[i]);

	iirfilt_rrrf_destroy(f);

/* done: */
	hanning(data, CHUNK_SIZE, data);

	/* overlapping */
	static float prev_data[CHUNK_SIZE / 2];

	for (i = 0; i < CHUNK_SIZE / 2; ++i)
		data[i] = data[i] + prev_data[i];

	/* save current */
	memcpy(prev_data, data + CHUNK_SIZE / 2, sizeof(prev_data));

	/* de-emphasis filter */
	lpc_de_emphasis_filter(data, output);
}
