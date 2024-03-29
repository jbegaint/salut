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

#ifndef _UTILS_H
#define _UTILS_H

#define UNUSED(x) (void)(x)

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define handle_alloc_error(x) if (!x) die("Could not allocate memory.\n")

/* usefull math macros (from kernel.h) */
#ifndef MIN
#define MIN(x, y) __extension__({               \
		typeof(x) _min1 = (x);                  \
		typeof(y) _min2 = (y);                  \
		_min1 < _min2 ? _min1 : _min2; 			\
		})
#endif

#ifndef MAX
#define MAX(x, y) __extension__({               \
		typeof(x) _max1 = (x);                  \
		typeof(y) _max2 = (y);                  \
		_max1 > _max2 ? _max1 : _max2;			\
		})
#endif

#ifndef CLAMP
#define CLAMP(x, low, high) __extension__({				\
		typeof(x) _x = (x);								\
		typeof(low) _low = (low);						\
		typeof(high) _high = (high);					\
		_x > _high ? _high : (_x < _low ? _low : _x);	\
		})
#endif

void die(const char *s, ...);
void errno_die(void);

float **allocate_2d_arrayf(int n, int m);
void free_2d_arrayf(float **data);

#endif
