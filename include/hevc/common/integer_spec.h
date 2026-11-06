#ifndef HEVC_COMMON_INTEGER_SPEC_H
#define HEVC_COMMON_INTEGER_SPEC_H

#include <stdint.h>

/* Clip3(lo, hi, value) from clause 5. The value is unchanged when it
 * already lies inside the closed interval. */
int32_t hevc_clip3(int32_t lo, int32_t hi, int32_t value);

/* Floor-divide value by 2^shift.
 * The specification writes this as an arithmetic right shift. A right
 * shift of a negative signed value is not portable C, so a negative value
 * is shifted as a non-negative magnitude and the quotient is negated.
 * shift is 0..30. A larger shift is not used by the clauses implemented
 * here; the function returns value unchanged in that case so a bad shift
 * cannot invent a result by shifting off the end of the word.
 */
int32_t hevc_floor_div_pow2(int32_t value, int32_t shift);

#endif
