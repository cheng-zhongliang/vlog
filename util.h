#ifndef __UTIL_H__
#define __UTIL_H__

#include <limits.h>

/* Saturating addition: overflow yields UINT_MAX. */
static inline unsigned int sat_add(unsigned int x, unsigned int y) {
    return x + y >= x ? x + y : UINT_MAX;
}

/* Saturating subtraction: underflow yields 0. */
static inline unsigned int sat_sub(unsigned int x, unsigned int y) {
    return x >= y ? x - y : 0;
}

/* Saturating multiplication: overflow yields UINT_MAX. */
static inline unsigned int sat_mul(unsigned int x, unsigned int y) {
    return (!y ? 0 : x <= UINT_MAX / y ? x * y : UINT_MAX);
}

#ifndef MIN
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

#ifndef MAX
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#endif

#define ARRAY_SIZE(ARRAY) (sizeof ARRAY / sizeof *ARRAY)

#endif