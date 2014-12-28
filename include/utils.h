#ifndef _UTILS_H
#define _UTILS_H

#define UNUSED(x) (void)(x)

/* usefull math macros (from kernel.h) */
#define min(x, y) __extension__({               \
		typeof(x) _min1 = (x);                  \
		typeof(y) _min2 = (y);                  \
		(void) (&_min1 == &_min2);				\
		_min1 < _min2 ? _min1 : _min2; })

#define max(x, y) __extension__({               \
		typeof(x) _max1 = (x);                  \
		typeof(y) _max2 = (y);                  \
		(void) (&_max1 == &_max2);				\
		_max1 > _max2 ? _max1 : _max2; })

void die(const char *s, ...);
void errno_die(void);

#endif
