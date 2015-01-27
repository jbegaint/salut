/*
 * Copyright (c) 2014 - 2015 Jean Bégaint
 *
 * This file is part of salut.
 *
 * salut is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * salut is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with salut.  If not, see <http://www.gnu.org/licenses/>.
 */

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
char *cb_try_get_rptr(CircularBuffer *cb);

void cb_increment_count(CircularBuffer *cb);

#endif
