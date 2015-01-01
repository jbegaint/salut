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
	float **data;

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
