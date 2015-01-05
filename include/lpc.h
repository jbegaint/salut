#ifndef _LPC_H_
#define _LPC_H_

#include "general.h"

#define CHUNK_SIZE BUF_SIZE

#define FS SAMPLE_RATE

/* in Hertz... */
#define MAX_FREQ 850
#define MIN_FREQ 50

/* in samples */
#define MIN_PITCH ((float) FS / (float) MAX_FREQ)
#define MAX_PITCH ((float) FS / (float) MIN_FREQ)

#define N_COEFFS 64
/* #define N_COEFFS 24 */

typedef struct {
	/*
	 * Pitch frequency:
	 * 	- N if voiced,
	 * 	- 0 if non voiced,
	 * 	- -1 if noise/silence.
	 */
	short pitch;
	float coefficients[N_COEFFS]; /* lpc coefficients */
} LpcChunk;

typedef struct {
	/* stereo or mono chunks */
	LpcChunk chunks[NUM_CHANNELS];
} LpcData;

void my_liquid_lpc(float * _x, unsigned int _n, unsigned int _p, float * _a,
		float * _g);

int lpc_data_serialize(LpcData *data, char *buf);
int lpc_data_deserialize(char *buf, LpcData *data);

void lpc_data_decode(LpcData *lpc_data, float **data);
void lpc_data_encode(float **data, LpcData *lpc_data);

#endif
