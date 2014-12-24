#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

#include "circbuf.h"

void cb_init(CircularBuffer *cb, int size, int elt_size)
{
	memset(cb, 0, sizeof(*cb));

	/* fill struct info */
	cb->size = size;
	cb->elt_size = elt_size;
	cb->start = 0;

	/* allocate memory */
	cb->data = calloc(cb->elt_size * size, sizeof(float));

	/* init the semaphore */
	sem_init(&cb->sem, 0, 10);
}

void cb_free(CircularBuffer *cb)
{
	sem_destroy(&cb->sem);

	/* free memory */
	free(cb->data);
}

float *cb_get_rptr(CircularBuffer *cb)
{
	sem_wait(&cb->sem);

	cb->start = (cb->start + 1) % cb->size;

	return &cb->data[cb->start * cb->elt_size];
}

float *cb_get_wptr(CircularBuffer *cb)
{
	int count;
	sem_getvalue(&cb->sem, &count);
	int end = (cb->start + count) % cb->size;

	if (count == cb->size) {
		/* cb is full, overwrite */
		cb->start = (cb->start + 1) % cb->size;
	}
	else {
		/* counter increment in the callback */
	}

	return  &cb->data[end * cb->elt_size];
}

void cb_increment_count(CircularBuffer *cb)
{
	if (sem_post(&cb->sem) == -1)
		/* FIXME */
		fprintf(stderr, "error...\n");
}
