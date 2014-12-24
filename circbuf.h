#ifndef _CIRCBUF_H_
#define _CIRCBUF_H_

#include <semaphore.h>

typedef struct {
	int size;
	int elt_size;
	int start;
	sem_t sem; /* semaphore as counter */
	float *data;
} CircularBuffer;

void cb_init(CircularBuffer *cb, int size, int elt_size);
void cb_free(CircularBuffer *cb);

float *cb_get_rptr(CircularBuffer *cb);
float *cb_get_wptr(CircularBuffer *cb);

void cb_increment_count(CircularBuffer *cb);

#endif