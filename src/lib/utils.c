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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "utils.h"

void die(const char *s, ...)
{
	va_list err;

	va_start(err, s);
	vfprintf(stderr, s, err);
	va_end(err);

	exit(EXIT_FAILURE);
}

void errno_die(void)
{
	die("[Error] %s (%d).\n", strerror(errno), errno);
}

float **allocate_2d_arrayf(int n, int m)
{
	float **data = NULL;

	/* allocate memory */
	data = calloc(n, sizeof(*data));
	handle_alloc_error(data);

	*data = calloc(n * m, sizeof(**data));
	handle_alloc_error(*data);

	for (int i = 1; i < n; ++i) {
		data[i] = data[i - 1] + m;
	}

	return data;
}

void free_2d_arrayf(float **data)
{
	free(*data);
	free(data);
}
