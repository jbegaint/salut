#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>

#include <portaudio.h>

#include "general.h"
#include "utils.h"
#include "lpc.h"

#define handle_pa_error(err) if (err != paNoError) goto done

typedef struct {
	int size;
	int start;
	/* unneeded counter, as we use the semaphore instead */
	/* unsigned int count; */
	float *data;
} CircularBuffer;

typedef struct {
	int *running;
	CircularBuffer *circbuf;
} Context;

Options options = {0.1f, 64};
int running = 1;

static sem_t sem;

static void circbuf_init(CircularBuffer *circbuf, int size)
{
	memset(circbuf, 0, sizeof(*circbuf));
	circbuf->size = size;
	circbuf->data = calloc(NUM_CHANNELS * BUF_SIZE * size, sizeof(float));
	circbuf->start = 0;
}
static float *circbuf_get_rptr(CircularBuffer *circbuf)
{
	circbuf->start = (circbuf->start + 1) % circbuf->size;

	/* counter decrement is done in the callback */

	return &circbuf->data[circbuf->start * BUF_SIZE * NUM_CHANNELS];
}

static float *circbuf_get_wptr(CircularBuffer *circbuf)
{
	int count;
	sem_getvalue(&sem, &count);
	int end = (circbuf->start + count) % circbuf->size;

	if (count == circbuf->size) {
		/* circbuf is full, overwrite */
		circbuf->start = (circbuf->start + 1) % circbuf->size;
	}
	else {
		/* counter increment in the callback */
	}

	return  &circbuf->data[end * BUF_SIZE * NUM_CHANNELS];
}

static void sigint_handler(int signum)
{
	(void) (signum);

	running = 0;
	fprintf(stderr, "Exiting...\n");
}

static int playCallback(const void *input, void *output,
						unsigned long frames_count,
						const PaStreamCallbackTimeInfo *timeInfo,
						PaStreamCallbackFlags status_flags,
						void *user_data)
{
	Context *ctx = (Context *) user_data;
	float *wptr = (float *) output;
	float *rptr = circbuf_get_rptr(ctx->circbuf);

	unsigned long i;

	(void) (input);
	(void) (status_flags);
	(void) (timeInfo);
	(void) (user_data);

	if (!*ctx->running)
		return paComplete;

	sem_wait(&sem);

	for (i = 0; i < frames_count; ++i) {
		/* left and right */
		*wptr++ = *rptr++;
		*wptr++ = *rptr++;
	}

	return paContinue;
}

static int recordCallback(const void *input, void *output,
						unsigned long frames_count,
						const PaStreamCallbackTimeInfo *timeInfo,
						PaStreamCallbackFlags status_flags,
						void *user_data)
{
	Context *ctx = (Context *) user_data;
	const float *rptr = (const float *) input;
	float *wptr = circbuf_get_wptr(ctx->circbuf);
	float **data = NULL;

	unsigned long i;
	LpcData out;

	(void) (output);
	(void) (status_flags);
	(void) (timeInfo);
	(void) (user_data);

	if (!*ctx->running)
		return paComplete;

	data = allocate_2d_arrayf(NUM_CHANNELS, CHUNK_SIZE);

	for (i = 0; i < frames_count; ++i) {
		/* left and right */
		data[0][i] = *rptr++;
		data[1][i] = *rptr++;
	}

	/* lpc stuff */
	lpc_data_encode(data, &out);
	lpc_data_decode(&out, data);

	for (i = 0; i < frames_count; ++i) {
		/* left and right */
		*wptr++ = data[0][i];
		*wptr++ = data[1][i];
	}

	sem_post(&sem);
	free_2d_arrayf(data);

	return paContinue;
}

int main(void)
{
	PaError err = paNoError;
	PaStream *inputStream;
	PaStream *outputStream;

	CircularBuffer circbuf;
	Context ctx;

	/* signal catching */
	signal(SIGINT, sigint_handler);

	/* init circular buffer */
	circbuf_init(&circbuf, 20);

	/* init context */
	ctx.running = &running;
	ctx.circbuf = &circbuf;
	ctx.circbuf->start = 0;

	sem_init(&sem, 0, 10);

	/* init portaudio */
	err = Pa_Initialize();
	handle_pa_error(err);

	/* open default input stream for playing */
	err = Pa_OpenDefaultStream(&inputStream, 2, 0, paFloat32, SAMPLE_RATE, 256,
			recordCallback, &ctx);
	handle_pa_error(err);

	/* open default output stream for playback */
	err = Pa_OpenDefaultStream(&outputStream, 0, 2, paFloat32, SAMPLE_RATE, 256,
			playCallback, &ctx);
	handle_pa_error(err);

	/* start streams */
	err = Pa_StartStream(inputStream);
	handle_pa_error(err);
	err = Pa_StartStream(outputStream);
	handle_pa_error(err);

	/* do stuff */
	printf("===\nPlease speak into the microphone.\n");

	/* FIXME */
	while ((err = Pa_IsStreamActive(inputStream)
				& Pa_IsStreamActive(outputStream)) == 1) {
		Pa_Sleep(100);
	}
	printf("\n");

	/* close streams */
	fprintf(stderr, "Closing streams...");

	err = Pa_CloseStream(inputStream);
	handle_pa_error(err);
	err = Pa_CloseStream(outputStream);
	handle_pa_error(err);

	fprintf(stderr, " done.\n");

done:
	/* terminate portaudio (MUST be called) */
	Pa_Terminate();

	sem_destroy(&sem);

	/* free memory */
	free((ctx.circbuf)->data);

	return err;
}
