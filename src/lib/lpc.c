#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <liquid/liquid.h>

#include "general.h"
#include "lpc.h"
#include "utils.h"

/* #define fprintf(x,...) (void)(x) */

extern Options options;

static void hanning(const float *input, const size_t len, float *output)
{
	unsigned int i;
	float coef;

	for (i = 0; i < len; ++i) {
		coef = 0.5f - 0.5f * cos(2.f * M_PI * i / len);
		*output++ = *input++ * coef;
	}
}

static int get_pitch_by_autocorr(float *input, const size_t len)
{
	int pitch = -1;
	unsigned int i, k;
	float sum = 0, old_sum, thresh;

	int state = 1;

	for ( k = 0; k < len; ++k) {
		sum += input[k] * input[k];
	}
	thresh = 0.3f * sum;

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

static void lpc_pre_emphasis_filter(float *input, float *output)
{
	unsigned int i;

	float a[2] = {1.f, 1.f};
	float b[2] = {1.f, -0.9f};

	iirfilt_rrrf f = iirfilt_rrrf_create(b, 2, a, 2);

	for (i = 0; i < CHUNK_SIZE; ++i)
		iirfilt_rrrf_execute(f, input[i], &output[i]);

	iirfilt_rrrf_destroy(f);
}

static void lpc_de_emphasis_filter(float *input, float *output)
{
	unsigned int i;

	float a[2] = {1.f, -0.9f};
	float b[2] = {1.f, 1.f};

	iirfilt_rrrf f = iirfilt_rrrf_create(b, 2, a, 2);

	for (i = 0; i < CHUNK_SIZE; ++i)
		iirfilt_rrrf_execute(f, input[i], &output[i]);

	iirfilt_rrrf_destroy(f);
}

LpcChunk lpc_encode(float *input)
{
	LpcChunk lpc_chunk;
	float data[CHUNK_SIZE];

	/* compute chunk energy */
	float e = 0, val;

	for (int i = 0; i < CHUNK_SIZE; ++i) {
		val = fabs(input[i] * input[i]);
		e = MAX(e, val);
	}

	/* background noise detection */
	if (e < options.energy_thresh) {
		lpc_chunk.pitch = -1;
		return lpc_chunk;
	}

	/* pre emphasis filter */
	lpc_pre_emphasis_filter(input, data);

	/* compute pitch (autocorr FTW) */
	const int pitch = get_pitch_by_autocorr(input, CHUNK_SIZE);

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

	/* basic "interface" */
	fprintf(stderr, "\r [%s] Voiced  [%s] Unvoiced  [%s] Background Noise",
			(pitch > 0) ? "x" : " ",
			(pitch == 0) ? "x" : " ",
			(pitch == -1) ? "x" : " ");

	/* compute the excitation */
	if (pitch > 0) {
		/* if voiced, generate an impulsion train */
		for (i = 0; i < CHUNK_SIZE; ++i)
			excitation[i] = ((i % pitch) == 0) ? 1 : 0;
	}
	else if (pitch == 0) {

		/*
		 * Uniform distribution (similar to matlab's rand()). Generate
		 * random values between -1 and 1 (*[max-min]+min).
		 */

		for (i = 0; i < CHUNK_SIZE; ++i)
			excitation[i] = ((float) rand() / (float) RAND_MAX) * 2 - 1;
	}
	else {

		/*
		 * Uniform distribution (similar to matlab's rand()). Generate
		 * random values between -1 and 1 (*[max-min]+min).
		 */

		/* (TODO: store some background noise in memory ?) */

		for (i = 0; i < CHUNK_SIZE; ++i)
			output[i] = (((float) rand() / (float) RAND_MAX) * 2 - 1) * 0.002f;

		/* all done */
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

	/* discard these coefficients ? */
	if (!stable)
		return;

	/* filter the excitation */
	iirfilt_rrrf f = iirfilt_rrrf_create(b_lpc, N_COEFFS, a_lpc, N_COEFFS);

	for (i = 0; i < CHUNK_SIZE; ++i)
		iirfilt_rrrf_execute(f, excitation[i], &data[i]);

	iirfilt_rrrf_destroy(f);

	/* apodization */
	hanning(data, CHUNK_SIZE, data);

	/* overlapping */
	static float prev_data[CHUNK_SIZE / 2];

	for (i = 0; i < CHUNK_SIZE / 2; ++i)
		data[i] = data[i] + prev_data[i];

	/* save current for next run */
	memcpy(prev_data, data + CHUNK_SIZE / 2, sizeof(prev_data));

	/* de-emphasis filter */
	lpc_de_emphasis_filter(data, output);
}

int lpc_data_serialize(LpcData *data, char *buf)
{
	int i, sz, pitch;
	int offset = 0;

	for (i = 0; i < NUM_CHANNELS; ++i) {

		pitch = data->chunks[i].pitch;
		sz = sizeof(pitch);

		/* write pitch */
		memcpy(buf + offset, &pitch, sz);
		offset += sz;

		/* write coefficients ? */
		if (pitch >= 0) {
			sz = N_COEFFS * sizeof(float);
			memcpy(buf + offset, data->chunks[i].coefficients, sz);
			offset += sz;
		}
	}

	return offset;
}

int lpc_data_deserialize(char *buf, LpcData *data)
{
	int i, sz, pitch;
	int offset = 0;

	for (i = 0; i < NUM_CHANNELS; ++i) {

		sz = sizeof(int);

		/* retrieve pitch */
		memcpy(&pitch, buf + offset, sz);
		offset += sz;

		data->chunks[i].pitch = pitch;

		/* retrieve coefficients ? */
		if (pitch >= 0) {
			sz = N_COEFFS * sizeof(float);
			memcpy(data->chunks[i].coefficients, buf + offset, sz);
			offset += sz;
		}
	}

	return offset;
}

void lpc_data_decode(LpcData *lpc_data, float **data)
{
	for (int i = 0; i < NUM_CHANNELS; ++i)
		lpc_decode(&lpc_data->chunks[i], data[i]);
}

void lpc_data_encode(float **data, LpcData *lpc_data)
{
	for (int i = 0; i < NUM_CHANNELS; ++i)
		lpc_data->chunks[i] = lpc_encode(data[i]);
}
