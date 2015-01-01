#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <portaudio.h>
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

#include "circbuf.h"
#include "context.h"
#include "general.h"
#include "lpc.h"
#include "udp.h"
#include "utils.h"

#define MAX_LEN 256

#define handle_pa_error(err) if (err != paNoError) goto done

static int running = 1;

static void on_quit(void)
{
	running = 0;

	fprintf(stderr, "Exiting...\n");
}

static void *handle_con_refused(void)
{
	if (errno == ECONNREFUSED) {
		/* peer disconnected */
		fprintf(stderr, "Peer disconnected.\n");

		/* let's exit */
		on_quit();

	}
	else {
		/* something terrible happend */
		errno_die();
	}

	return NULL;
}

static void sigint_handler(int signum)
{
	UNUSED(signum);

	on_quit();
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

	if (!(*ctx->running))
		return paComplete;

	/* get pointer to readable circbuf data */
	rptr = (float *) cb_get_rptr(ctx->cb_in);

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

	if (!(*ctx->running))
		return paComplete;

	/* get pointer to writable circbuf data */
	wptr = (float *) cb_get_wptr(ctx->cb_out);

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

	return paContinue;
}

static void *send_thread_routine(void *arg)
{
	Context *ctx = (Context *) arg;
	LpcData out;
	char sendbuf[sizeof(LpcData)];
	float **data;
	float *rptr = NULL;
	int s = *ctx->socket_fd;
	int sz;

	data = allocate_2d_arrayf(NUM_CHANNELS, CHUNK_SIZE);

	while (*ctx->running) {

		/* do not get lost here while we are waiting for a buffer */
		do {
			rptr = (float *) cb_try_get_rptr(ctx->cb_out);
		} while (rptr == NULL && *ctx->running);

		if (!*ctx->running)
			break;

		/* get interleaved data */
		for (int i = 0; i < CHUNK_SIZE; ++i) {
			/* left and right */
			for (int j = 0; j < NUM_CHANNELS; ++j) {
				data[j][i] = *rptr++;
			}
		}

		/* LPC encoding */
		lpc_data_encode(data, &out);

		/* serialize data */
		sz = lpc_data_serialize(&out, sendbuf);

		/* send data */
		if (send(s, sendbuf, sz, 0) == -1)
			return handle_con_refused();
	}

	/* clean up */
	free_2d_arrayf(data);

	return NULL;
}

static void *read_thread_routine(void *arg)
{
	Context *ctx = (Context *) arg;
	LpcData in;
	char recvbuf[sizeof(LpcData)];
	float **data;
	float *wptr = NULL;
	int s = *ctx->socket_fd;
	int sel;

	data = allocate_2d_arrayf(NUM_CHANNELS, CHUNK_SIZE);

	/* do stuff */
	while (*ctx->running) {
		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(s, &fd);

		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 10000;

		do {
			sel = select(s + 1, &fd, NULL, NULL, &tv);
		} while ((sel == -1) && (errno == EINTR) && *ctx->running);

		if (!*ctx->running)
			break;

		if (sel == -1)
			errno_die();

		if (FD_ISSET(s, &fd)) {

			/* read incoming data */
			if (recv(s, recvbuf, sizeof(recvbuf), 0) == -1)
				return handle_con_refused();

			/* decode data */
			lpc_data_deserialize(recvbuf, &in);
			lpc_data_decode(&in, data);

			/* get a pointer to a writable zone in the circular buffer */
			wptr = (float *) cb_get_wptr(ctx->cb_in);

			/* write data (interleaved) */
			for (int i = 0; i < CHUNK_SIZE; ++i) {
				/* left and right */
				for (int j = 0; j < NUM_CHANNELS; ++j) {
					*wptr++ = data[j][i];
				}
			}

			/* all done, data is written */
			cb_increment_count(ctx->cb_in);
		}

		/* DEBUG */
		/* int vin, vout; */
		/* sem_getvalue(&ctx->cb_in->sem, &vin); */
		/* sem_getvalue(&ctx->cb_out->sem, &vout); */
		/* fprintf(stderr, "\rSem values: in [%d] out [%d]", vin, vout); */
	}

	/* clean up */
	free_2d_arrayf(data);

	return NULL;
}

int main(int argc, char **argv)
{
	PaError err = paNoError;
	PaStream *inputStream;
	PaStream *outputStream;

	Context *ctx;
	pthread_t read_thread, send_thread;

	int rc, socket_fd;
	struct sockaddr_in myaddr;
	struct sockaddr_in peeraddr;
	struct timeval tv;
	socklen_t socklen = sizeof(struct sockaddr_in);

	/* signal catching */
	signal(SIGINT, sigint_handler);

	/* init udp socket */
	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_fd == -1)
		errno_die();

	const char *msg = "Hello World!";
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

	tv.tv_sec = 5;
	tv.tv_usec = 0;

	/* set socket timeout option */
	if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		errno_die();
	}

	/* initial handshake */
	if (argc == 1) {
		/* server mode */
		fprintf(stderr, "Server mode...\n");

		/* waiting for a peer */
		rc = recv_msg(socket_fd, &peeraddr, buffer, MAX_LEN);

		if (rc == -1) {
			fprintf(stderr, "No response from peer, exiting.\n");
			close(socket_fd);
			exit(EXIT_FAILURE);
		}

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

		/* sending hello message */
		send_msg(socket_fd, &peeraddr, msg, strlen(msg) + 1);

		/* waiting for the peer to respond */
		rc = recvfrom(socket_fd, buffer, buf_len, 0, (struct sockaddr *) &peeraddr, &socklen);

		if (rc == -1) {
			fprintf(stderr, "No response from peer, exiting.\n");
			close(socket_fd);
			exit(EXIT_FAILURE);
		}

		printf("[MSG FROM PEER]: %s\n", buffer);

	}
	/* connecting to peer */
	if (connect(socket_fd, (struct sockaddr *) &peeraddr, socklen) == -1)
		errno_die();

	/* init context */
	ctx = ctx_init(&running, 20);
	ctx->socket_fd = &socket_fd;
	ctx->peeraddr = &peeraddr;
	ctx->myaddr = &myaddr;

	/* init portaudio */
	err = Pa_Initialize();
	handle_pa_error(err);

	/* open default input stream for playing */
	err = Pa_OpenDefaultStream(&inputStream, 2, 0, paFloat32, SAMPLE_RATE,
			BUF_SIZE, recordCallback, ctx);
	handle_pa_error(err);

	/* open default output stream for playback */
	err = Pa_OpenDefaultStream(&outputStream, 0, 2, paFloat32, SAMPLE_RATE,
			BUF_SIZE, playCallback, ctx);
	handle_pa_error(err);

	/* start streams */
	err = Pa_StartStream(inputStream);
	handle_pa_error(err);
	err = Pa_StartStream(outputStream);
	handle_pa_error(err);

	/* do stuff */
	printf("===\nPlease speak into the microphone.\n");

	/* start threads for receiving and sending data */
	if (pthread_create(&read_thread, NULL, read_thread_routine, ctx) != 0)
		errno_die();

	if (pthread_create(&send_thread, NULL, send_thread_routine, ctx) != 0)
		errno_die();

	/* FIXME */
	while ((err = Pa_IsStreamActive(inputStream)
				& Pa_IsStreamActive(outputStream)) == 1) {
		Pa_Sleep(100);
	}
	printf("\n");

	/* close streams */
	err = Pa_CloseStream(inputStream);
	handle_pa_error(err);
	err = Pa_CloseStream(outputStream);
	handle_pa_error(err);

done:
	/* terminate portaudio (MUST be called) */
	Pa_Terminate();

	/* waiting for the threads to finish */
	if (pthread_join(read_thread, NULL) != 0)
		errno_die();

	if (pthread_join(send_thread, NULL) != 0)
		errno_die();

	/* clean up */
	close(socket_fd);
	ctx_free(ctx);

	return err;
}
