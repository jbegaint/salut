#ifndef _LPC_H_
#define _LPC_H_

/* FIXME */
/* apodization window size (in samples, for Fs = 22050Hz) */
/* #define WINDOW_SIZE 256 */
#define WINDOW_SIZE 512

/*
 * Threshold for voiced detection. We use fmax ~= 300 Hz, ie 300/Fs in samples.
 */
#define F_THRESHOLD 0.014

#define N_COEFFS 64

typedef struct {
	float pitch;
	float coefficients[N_COEFFS]; /* lpc coefficients */
} LpcChunk;

typedef struct {
	unsigned int n_chunks;
	unsigned int chunk_size;

	LpcChunk *chunks;
} LpcData;

LpcData *lpc_data_init(const size_t input_len);
void lpc_data_free(LpcData *lpc_data);

LpcData *lpc_encode(float *input, const size_t input_len, size_t *sz);
void lpc_decode(LpcData *lpc_data, float *output);

void lpc_detect_voiced(float *input, LpcData *lpc_data);

#endif
