#include <arpa/inet.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>

#include <portaudio.h>

#include "circbuf.h"
#include "udp.h"
#include "utils.h"

/* #define SAMPLE_RATE 44100 */
#define SAMPLE_RATE 22050
#define NUM_CHANNELS 2
#define SAMPLE_SILENCE 0.0f

#define BUF_SIZE 256

#define handle_pa_error(err) if (err != paNoError) goto done

typedef struct {
	int *running;
	CircularBuffer *cb;
} Context;

static int running = 1;

static void sigint_handler(int signum)
{
	UNUSED(signum);

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
	float *rptr = NULL;

	unsigned long i;

	UNUSED(input);
	UNUSED(status_flags);
	UNUSED(timeInfo);
	UNUSED(user_data);

	if (!*ctx->running)
		return paComplete;

	/* get pointer to readable circbuf data */
	rptr = cb_get_rptr(ctx->cb);

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
	float *wptr = NULL;

	unsigned long i;

	UNUSED(output);
	UNUSED(status_flags);
	UNUSED(timeInfo);
	UNUSED(user_data);

	if (!*ctx->running)
		return paComplete;

	/* DEBUG */
	int value;
	sem_getvalue(&ctx->cb->sem, &value);
	fprintf(stderr, "\rSem value: [%d]", value);

	/* get pointer to writale data circbuf data */
	wptr = cb_get_wptr(ctx->cb);

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

	/* all done */
	cb_increment_count(ctx->cb);

	return paContinue;
}

int main(int argc, char **argv)
{
	PaError err = paNoError;
	PaStream *inputStream;
	PaStream *outputStream;

	CircularBuffer cb;
	Context ctx;

	int socket_fd;
	struct sockaddr_in myaddr;
	struct sockaddr_in peeraddr;
	socklen_t socklen = sizeof(struct sockaddr_in);

	/* signal catching */
	signal(SIGINT, sigint_handler);

	/* init udp socket */
	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_fd == -1)
		errno_die();

	const char *msg = "Hello World!";
	int rc;
	int buf_len = 256;
	char buffer[256];

	init_peer_addr(&myaddr, INADDR_ANY);

	/* LOCALHOST TESTING ONLY */
	if (argc != 1) {
		/* client mode */
		myaddr.sin_port = DFLT_PORT + 1;
	}

	/* bind socket */
	if (bind(socket_fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0)
		errno_die();

	if (argc == 1) {
		/* server mode */
		fprintf(stderr, "Server mode...\n");

		fprintf(stderr, "wainting for msg...\n");

		recvfrom(socket_fd, buffer, buf_len, 0, (struct sockaddr *) &peeraddr, &socklen);
		printf("[MSG FROM CLIENT]: %s\n", buffer);

		sendto(socket_fd, buffer, buf_len, 0, (struct sockaddr *) &peeraddr, socklen);
		send_msg(socket_fd, &peeraddr, buffer, strlen(buffer) + 1);
	}
	else {
		/* client mode */
		fprintf(stderr, "Client mode...\n");

		/* FIXME: USE ARGV */
		char *host_name = "127.0.0.1";
		struct hostent *hp;
		hp = gethostbyname(host_name);

		init_peer_addr(&peeraddr, INADDR_ANY);
		/* copy peer addr */
		memcpy(&peeraddr.sin_addr, hp->h_addr_list[0], hp->h_length);

		fprintf(stderr, "sending msg...\n");

		rc = send_msg(socket_fd, &peeraddr, msg, strlen(msg) + 1);
		if (rc < 0)
			errno_die();

		recvfrom(socket_fd, buffer, buf_len, 0, (struct sockaddr *) &peeraddr, &socklen);
		printf("[MSG FROM SERVER]: %s\n", buffer);
	}

	close(socket_fd);
	exit(EXIT_SUCCESS);

	/* init circular buffer */
	cb_init(&cb, 20, NUM_CHANNELS * BUF_SIZE);

	/* init context */
	ctx.running = &running;
	ctx.cb = &cb;

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

	close(socket_fd);

	cb_free(ctx.cb);

	return err;
}
