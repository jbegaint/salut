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

#ifndef _GENERAL_H_
#define _GENERAL_H_

#ifndef SAMPLE_RATE
/* #define SAMPLE_RATE 44100 */
/* #define SAMPLE_RATE 22050 */
#define SAMPLE_RATE 16000
/* #define SAMPLE_RATE 11025 */
#endif

#define SAMPLE_SILENCE 0.0f

#define NUM_CHANNELS 2

#ifndef BUF_SIZE
/* #define BUF_SIZE 256 */
#define BUF_SIZE 512
#endif

typedef struct {
	/* threshold for background noise detection */
	float energy_thresh;

	/* number of lpc coefficients */
	int n_coeffs;
} Options;

#endif
