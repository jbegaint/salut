#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

#include "circbuf.h"
#include "utils.h"

CircularBuffer *cb_init(int size, int elt_size)
{
	CircularBuffer *cb = calloc(1, sizeof(*cb));

	/* fill struct info */
	cb->size = size;
	cb->elt_size = elt_size;
	cb->start = 0;

	/* allocate memory */
	cb->data = calloc(cb->elt_size * size, sizeof(float));

	/* init the semaphore */
	if (sem_init(&cb->sem, 0, 10) == -1)
		errno_die();

	return cb;
}

void cb_free(CircularBuffer *cb)
{
	if (sem_destroy(&cb->sem) == -1)
		errno_die();

	/* free memory */
	free(cb->data);
}

float *cb_get_rptr(CircularBuffer *cb)
{
	if (sem_wait(&cb->sem) == -1)
		errno_die();

	cb->start = (cb->start + 1) % cb->size;

	return &cb->data[cb->start * cb->elt_size];
}

float *cb_get_wptr(CircularBuffer *cb)
{
	int count, end;

	if (sem_getvalue(&cb->sem, &count) == -1)
		errno_die();

	/* compute buffer end */
	end = (cb->start + count) % cb->size;

	if (count == cb->size) {
		/* cb is full, overwrite */
		cb->start = (cb->start + 1) % cb->size;
	}

	/*
	 * Counter increment is called in the callback, once all the data has been
	 * written to the buffer.
	 */

	return  &cb->data[end * cb->elt_size];
}

void cb_increment_count(CircularBuffer *cb)
{
	if (sem_post(&cb->sem) == -1)
		errno_die();
}
