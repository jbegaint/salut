#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>

#include <portaudio.h>

/* #define SAMPLE_RATE 44100 */
#define SAMPLE_RATE 22050
#define NUM_SECONDS 2
#define NUM_CHANNELS 2
#define SAMPLE_SILENCE 0.0f

#define BUF_SIZE 256

#define handle_pa_error(err) if (err != paNoError) goto done

typedef struct {
	int size;
	int start;
	/* unneeded counter, as we use the semaphore instead */
	/* unsigned int count; */
	float *end;
	float *data;
} CircularBuffer;

typedef struct {
	int *running;
	CircularBuffer *cb;
} Context;

static int running = 1;
static sem_t sem;

static void cb_init(CircularBuffer *cb, int size)
{
	memset(cb, 0, sizeof(*cb));
	cb->size = size;
	cb->data = calloc(BUF_SIZE * size, sizeof(float));
	cb->start = 0;
}
static float *cb_get_rptr(CircularBuffer *cb)
{
	sem_wait(&sem);
	cb->start = (cb->start + 1) % cb->size;

	return &cb->data[cb->start * BUF_SIZE];
}

static float *cb_get_wptr(CircularBuffer *cb)
{
	int count;
	int end = (cb->start + sem_getvalue(&sem, &count)) % cb->size;

	if (count == cb->size) {
		/* cb is full, overwrite */
		cb->start = (cb->start + 1) % cb->size;
		fprintf(stderr, "OVERWRITE\n");
	}
	else {
		sem_post(&sem);
	}

	return  &cb->data[end * BUF_SIZE];
}

static void die(const char *s, ...)
{
	va_list err;

	va_start(err, s);
	vfprintf(stderr, s, err);
	va_end(err);

	exit(EXIT_FAILURE);
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
	float *rptr = cb_get_rptr(ctx->cb);

	unsigned long i;

	(void) (input);
	(void) (status_flags);
	(void) (timeInfo);
	(void) (user_data);

	if (!*ctx->running)
		return paComplete;

	for (i = 0; i < frames_count; ++i) {
		/* left and right */
		*wptr++ = *rptr;
		*wptr++ = *rptr;
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
	float *wptr = cb_get_wptr(ctx->cb);

	unsigned long i;

	(void) (output);
	(void) (status_flags);
	(void) (timeInfo);
	(void) (user_data);

	if (!*ctx->running)
		return paComplete;

	if (!input) {
		for (i = 0; i < frames_count; ++i) {
			/* left and right */
			*wptr++ = SAMPLE_SILENCE;
			*wptr++ = SAMPLE_SILENCE;
		}
	}
	else {
		for (i = 0; i < frames_count; ++i) {
			/* left and right */
			*wptr++ = *rptr++;
			*wptr++ = *rptr++;
		}
	}

	return paContinue;
}

int main(void)
{
	PaError err = paNoError;
	PaStream *inputStream;
	PaStream *outputStream;

	CircularBuffer cb;
	Context ctx;

	/* signal catching */
	signal(SIGINT, sigint_handler);

	/* init circular buffer */
	cb_init(&cb, 10 * SAMPLE_RATE * NUM_CHANNELS);

	/* init context */
	ctx.running = &running;
	ctx.cb = &cb;

	sem_init(&sem, 0, 2 * SAMPLE_RATE);

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

	while ((err = Pa_IsStreamActive(inputStream)) == 1) {
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

	free((ctx.cb)->data);

	sem_destroy(&sem);

	return err;
}
