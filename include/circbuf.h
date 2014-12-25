#ifndef _CIRCBUF_H_
#define _CIRCBUF_H_

#include <semaphore.h>

typedef struct {
	int size;
	int elt_size;
	int start;
	sem_t sem; /* semaphore as counter */
	/* float *data; */
	char *data;
} CircularBuffer;

CircularBuffer *cb_init(int size, int elt_size);
void cb_free(CircularBuffer *cb);

char *cb_get_rptr(CircularBuffer *cb);
char *cb_get_wptr(CircularBuffer *cb);

void cb_increment_count(CircularBuffer *cb);

#endif
