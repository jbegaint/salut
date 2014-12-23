#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

#include <portaudio.h>

#define SAMPLE_RATE 44100
#define NUM_SECONDS 2
#define NUM_CHANNELS 2
#define SAMPLE_SILENCE 0.0f

#define handle_pa_error(err) if (err != paNoError) goto done

typedef struct {
	float *samples;
	unsigned int frame_index;
	unsigned max_frame_index;
	int *running;
} paData;

static int running = 1;

static void die(const char *s, ...)
{
	va_list err;

	va_start(err, s);
	vfprintf(stderr, s, err);
	va_end(err);

	exit(EXIT_FAILURE);
}

/* static void pa_die(const PaError err) */
/* { */
/* 	die("PortAudio error: %s (%d).\n", Pa_GetErrorText(err), err); */
/* } */

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
	paData *data = (paData *) user_data;
	float *r_ptr = &data->samples[data->frame_index * NUM_CHANNELS];
	float *w_ptr = (float *) output;
	int finished;
	unsigned long i;
	unsigned int frames_left = data->max_frame_index - data->frame_index;

	(void) (input);
	(void) (status_flags);
	(void) (timeInfo);
	(void) (user_data);

	if (frames_left < frames_count) {
		/* this is the final buffer... */
		for (i = 0; i < frames_left; ++i) {
			/* left */
			*w_ptr++ = *r_ptr++;
			/* right */
			*w_ptr++ = *r_ptr++;
		}
		for (; i < frames_count; ++i) {
			/* left */
			*w_ptr++ = 0;
			/* right */
			*w_ptr++ = 0;
		}
		data->frame_index += frames_left;
		finished = paComplete;
	}
	else {
		for (i = 0; i < frames_count; ++i) {
			/* left */
			*w_ptr++ = *r_ptr++;
			/* right */
			*w_ptr++ = *r_ptr++;
		}
		data->frame_index += frames_count;
		finished = paContinue;
	}

	return finished;
}

static int recordCallback(const void *input, void *output,
						unsigned long frames_count,
						const PaStreamCallbackTimeInfo *timeInfo,
						PaStreamCallbackFlags status_flags,
						void *user_data)
{
	paData *data = (paData *) user_data;
	const float *r_ptr = (const float *) input;
	float *w_ptr = &data->samples[data->frame_index * NUM_CHANNELS];

	unsigned long frames_left = data->max_frame_index - data->frame_index;
	unsigned long frames_to_calc;
	int finished;
	unsigned long i;

	(void) (output);
	(void) (status_flags);
	(void) (timeInfo);
	(void) (user_data);

	/* number of frames to compute */
	if (frames_left < frames_count) {
		frames_to_calc = frames_left;
		finished = paComplete;
	}
	else {
		frames_to_calc = frames_count;
		finished = paContinue;
	}

	if (input == NULL) {
		for (i = 0; i < frames_to_calc; ++i) {
			/* left */
			*w_ptr++ = SAMPLE_SILENCE;
			/* right */
			*w_ptr++ = SAMPLE_SILENCE;
		}
	}
	else {
		for (i = 0; i < frames_to_calc; ++i) {
			/* left */
			*w_ptr++ = *r_ptr++;
			/* right */
			*w_ptr++ = *r_ptr++;
		}
	}

	data->frame_index += frames_to_calc;

	return finished;
}

int main(void)
{
	PaError err = paNoError;
	PaStream *inputStream;
	PaStream *outputStream;
	paData data;

	/* signal catching */
	signal(SIGINT, sigint_handler);

	/* init data struct */
	data.frame_index = 0;
	data.max_frame_index = SAMPLE_RATE * NUM_SECONDS;
	data.samples = calloc(data.max_frame_index * NUM_CHANNELS, sizeof(float));
	if (!data.samples) die("Could not allocate memory.\n");

	/* init portaudio */
	err = Pa_Initialize();
	handle_pa_error(err);

	/* open default input stream for playing */
	/* 256 or paFramesPerBufferUnspecified ? */
	err = Pa_OpenDefaultStream(&inputStream, 2, 0, paFloat32, SAMPLE_RATE, 256,
			recordCallback, &data);

	/* open default output stream for playback */
	err = Pa_OpenDefaultStream(&outputStream, 0, 2, paFloat32, SAMPLE_RATE, 256,
			playCallback, &data);
	handle_pa_error(err);

	err = Pa_StartStream(inputStream);
	handle_pa_error(err);

	printf("=== recording, please speak into the microphone. ===\n");

	while ((err = Pa_IsStreamActive(inputStream)) == 1) {
		Pa_Sleep(1000);
		printf(".");
		fflush(stdout);
	}
	printf("\n");

	err = Pa_CloseStream(inputStream);
	handle_pa_error(err);

	data.frame_index = 0;
	err = Pa_StartStream(outputStream);
	handle_pa_error(err);

	if (outputStream) {
		while ((err = Pa_IsStreamActive(outputStream)) == 1) {
			Pa_Sleep(100);
		}
		handle_pa_error(err);

		err = Pa_CloseStream(outputStream);
		handle_pa_error(err);

		printf("done.\n");
	}

done:
	/* terminate portaudio (MUST be called) */
	Pa_Terminate();

	return err;
}
