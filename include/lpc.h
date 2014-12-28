#ifndef _LPC_H_
#define _LPC_H_

#include "general.h"

/* FIXME */
/* apodization window size (in samples, for Fs = 22050Hz) */
/* #define WINDOW_SIZE 256 */
#define WINDOW_SIZE 512

#define CHUNK_SIZE 512

/*
 * Threshold for voiced detection. We use fmax ~= 300 Hz, ie 300/Fs in samples.
 */
#define FS SAMPLE_RATE
#define F_THRESHOLD 0.014

#define N_COEFFS 64

typedef struct {
	float pitch;
	float coefficients[N_COEFFS]; /* lpc coefficients */
} LpcChunk;

LpcChunk lpc_encode(float *input);
void lpc_decode(LpcChunk *lpc_chunk, float *output);

#endif
