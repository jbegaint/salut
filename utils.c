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
