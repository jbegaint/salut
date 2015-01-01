#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>

#include "circbuf.h"
#include "utils.h"

CircularBuffer *cb_init(int size, int elt_size)
{
	CircularBuffer *cb = calloc(1, sizeof(*cb));

	cb->size = size;
	cb->elt_size = elt_size;

	cb->data = calloc(cb->elt_size * size, sizeof(char));
	handle_alloc_error(cb->data);

	if (sem_init(&cb->sem, 0, 10) == -1)
		errno_die();

	return cb;
}

void cb_free(CircularBuffer *cb)
{
	if (sem_destroy(&cb->sem) == -1)
		errno_die();

	free(cb->data);
}

char *cb_get_rptr(CircularBuffer *cb)
{
	if (sem_wait(&cb->sem) == -1)
		errno_die();

	cb->start = (cb->start + 1) % cb->size;

	return &cb->data[cb->start * cb->elt_size];
}

char *cb_try_get_rptr(CircularBuffer *cb)
{
	struct timespec timeout;

	if (clock_gettime(CLOCK_REALTIME, &timeout) == 1)
		errno_die();

	/*
	 * Compute the timeout, as `sem_timedwait` takes a absolute time since the
	 * Epoch as argument.
	 */

	timeout.tv_sec += 1;

	if (sem_timedwait(&cb->sem, &timeout) == -1) {
		if (errno == ETIMEDOUT)
			return NULL;
		else
			errno_die();
	}

	cb->start = (cb->start + 1) % cb->size;

	return &cb->data[cb->start * cb->elt_size];
}

char *cb_get_wptr(CircularBuffer *cb)
{
	int count, end;

	if (sem_getvalue(&cb->sem, &count) == -1)
		errno_die();

	end = (cb->start + count) % cb->size;

	/* if cb is full, then overwrite */
	if (count == cb->size) {
		cb->start = (cb->start + 1) % cb->size;
	}

	/*
	 * Counter increment is called in the callback, once all the data has been
	 * written to the buffer.
	 */

	return &cb->data[end * cb->elt_size];
}

void cb_increment_count(CircularBuffer *cb)
{
	if (sem_post(&cb->sem) == -1)
		errno_die();
}
