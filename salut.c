#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
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
	int *socket_fd;

	CircularBuffer *cb_in, *cb_out;

	struct sockaddr_in *myaddr, *peeraddr;
} Context;

static int running = 1;
/* static pthread_mutex_t lock; */

static void ctx_init(Context *ctx, int *running_ptr, int n_elt)
{
	CircularBuffer *cb_in, *cb_out;

	/* init circular buffers */
	cb_in = cb_init(n_elt, NUM_CHANNELS * BUF_SIZE);
	cb_out = cb_init(n_elt, NUM_CHANNELS * BUF_SIZE);

	/* init context */
	ctx->running = running_ptr;
	ctx->cb_in = cb_in;
	ctx->cb_out = cb_out;
}

static void ctx_free(Context *ctx)
{
	/* free circular buffers */
	cb_free(ctx->cb_in);
	cb_free(ctx->cb_out);

	/* free context */
	free(ctx);
}

static void sigint_handler(int signum)
{
	UNUSED(signum);

	/* pthread_mutex_lock(&lock); */
	running = 0;
	/* pthread_mutex_unlock(&lock); */

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
	rptr = cb_get_rptr(ctx->cb_in);

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
	int vin, vout;
	sem_getvalue(&ctx->cb_in->sem, &vin);
	sem_getvalue(&ctx->cb_out->sem, &vout);
	fprintf(stderr, "\rSem values: in [%d] out [%d]", vin, vout);

	/* get pointer to writable circbuf data */
	wptr = cb_get_wptr(ctx->cb_out);

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
	cb_increment_count(ctx->cb_out);

	/* /1* send data *1/ */
	/* rptr = cb_get_rptr(ctx->cb_out); */
	/* /1* FIXME: LPC ? *1/ */

	/* send_msg(*ctx->socket_fd, ctx->peeraddr, rptr, sizeof(float) * ctx->cb_out->elt_size); */

	return paContinue;
}

static void *udp_thread_routine(void *arg)
{
	Context *ctx = (Context *) arg;
	int s = *ctx->socket_fd;
	float *wptr = NULL, *rptr = NULL;
	int rc, sn, sel;

	size_t sz = ctx->cb_in->elt_size;

	while (*ctx->running) {
		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(s, &fd);

		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 100;

		sel = select(s + 1, &fd, NULL, NULL, &tv);

		if (sel == -1) {
			if (errno != EINTR) {
				errno_die();
			}
		}
		else if (sel > 0) {
			/* read received data */
			wptr = cb_get_wptr(ctx->cb_in);

			rc = recv_msg(s, ctx->peeraddr, wptr, sizeof(float) * sz);
			/* FIXME: error on rc */
			UNUSED(rc);

			/* copy data */
			cb_increment_count(ctx->cb_in);
		}
		else {
			/* send data */
			int value;
			sem_getvalue(&ctx->cb_out->sem, &value);
			if (value == 0)
				continue;
			rptr = cb_get_rptr(ctx->cb_out);
			/* FIXME: LPC ? */

			send_msg(*ctx->socket_fd, ctx->peeraddr, rptr, sizeof(float) * ctx->cb_out->elt_size);
		}
	}

	return NULL;
}

int main(int argc, char **argv)
{
	PaError err = paNoError;
	PaStream *inputStream;
	PaStream *outputStream;

	Context *ctx = calloc(1, sizeof(*ctx));
	pthread_t udp_thread;

	int socket_fd;
	struct sockaddr_in myaddr;
	struct sockaddr_in peeraddr;

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

		fprintf(stderr, "waiting for msg...\n");

		recv_msg(socket_fd, &peeraddr, buffer, buf_len);
		printf("[MSG FROM CLIENT]: %s\n", buffer);

		send_msg(socket_fd, &peeraddr, buffer, strlen(buffer) + 1);
	}
	else {
		/* client mode */
		fprintf(stderr, "Client mode...\n");

		/* FIXME: TEST ARGV */
		char *host_name = argv[1];
		struct hostent *hp;
		hp = gethostbyname(host_name);

		init_peer_addr(&peeraddr, INADDR_ANY);
		/* copy peer addr */
		memcpy(&peeraddr.sin_addr, hp->h_addr_list[0], hp->h_length);

		fprintf(stderr, "sending msg...\n");

		rc = send_msg(socket_fd, &peeraddr, msg, strlen(msg) + 1);
		if (rc < 0)
			errno_die();

		recv_msg(socket_fd, &peeraddr, buffer, buf_len);
		printf("[MSG FROM SERVER]: %s\n", buffer);
	}

	/* init context */
	ctx_init(ctx, &running, 20);
	ctx->socket_fd = &socket_fd;
	ctx->peeraddr = &peeraddr;
	ctx->myaddr = &myaddr;

	/* init portaudio */
	err = Pa_Initialize();
	handle_pa_error(err);

	/* open default input stream for playing */
	err = Pa_OpenDefaultStream(&inputStream, 2, 0, paFloat32, SAMPLE_RATE, 256,
			recordCallback, ctx);
	handle_pa_error(err);

	/* open default output stream for playback */
	err = Pa_OpenDefaultStream(&outputStream, 0, 2, paFloat32, SAMPLE_RATE, 256,
			playCallback, ctx);
	handle_pa_error(err);

	/* start streams */
	err = Pa_StartStream(inputStream);
	handle_pa_error(err);
	err = Pa_StartStream(outputStream);
	handle_pa_error(err);

	/* do stuff */
	printf("===\nPlease speak into the microphone.\n");

	/* start connection thread */
	/* if (pthread_mutex_init(&lock, NULL) != 0) */
	/* 	errno_die(); */

	if (pthread_create(&udp_thread, NULL, udp_thread_routine, ctx) != 0)
		errno_die();

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

	if (pthread_join(udp_thread, NULL) != 0)
		errno_die();
	/* if (pthread_mutex_destroy(&lock) != 0) */
	/* 	errno_die(); */

	fprintf(stderr, "Closing socket...");
	close(socket_fd);
	fprintf(stderr, " done.\n");

	fprintf(stderr, "Freeing context...");
	ctx_free(ctx);
	fprintf(stderr, " done.\n");

	return err;
}
