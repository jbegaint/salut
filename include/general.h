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
#define BUF_SIZE 256
/* #define BUF_SIZE 512 */
/* #define BUF_SIZE 1024 */
#endif

#endif
