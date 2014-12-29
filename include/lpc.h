#ifndef _LPC_H_
#define _LPC_H_

#include "general.h"

#define CHUNK_SIZE BUF_SIZE

/*
 * Threshold for voiced detection. We use fmax ~= 300 Hz, ie 300/Fs in samples.
 */

#define FS SAMPLE_RATE

#define F_THRESHOLD (300 / FS)

/* in Hertz... */
#define MAX_FREQ 300
#define MIN_FREQ 50

/* in samples */
#define MIN_PITCH ((float) FS / (float) MAX_FREQ)
#define MAX_PITCH ((float) FS / (float) MIN_FREQ)

#define N_COEFFS 64
/* #define N_COEFFS 24 */

typedef struct {
	/* Pitch frequency. N if voiced, 0 if non voiced, -1 if noise/silence */
	int pitch;
	float coefficients[N_COEFFS]; /* lpc coefficients */
} LpcChunk;

typedef struct {
	/* left and right chunks */
	LpcChunk chunks[2];
} LpcData;

LpcChunk lpc_encode(float *input);
void lpc_decode(LpcChunk *lpc_chunk, float *output);

void my_liquid_lpc(float * _x, unsigned int _n, unsigned int _p, float * _a,
		float * _g);

#endif
