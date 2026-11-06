#include "hevc/common/integer_spec.h"

/* Clip one specification integer into a closed interval. */
int32_t hevc_clip3(int32_t lo, int32_t hi, int32_t value)
{
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

/* Arithmetic right shift, written so the shifted word is non-negative. */
int32_t hevc_floor_div_pow2(int32_t value, int32_t shift)
{
    uint32_t magnitude;
    uint32_t round;
    uint32_t quotient;

    if (shift <= 0 || shift > 30) {
        return value;
    }
    if (value >= 0) {
        /* The value is non-negative, so this shift matches floor division. */
        return value >> shift;
    }

    /* Magnitude of a negative value. Subtracting the unsigned bit pattern
     * avoids a signed negation of the most negative int32. */
    magnitude = (uint32_t)0 - (uint32_t)value;
    /* ceil(magnitude / 2^shift) = (magnitude + 2^shift - 1) / 2^shift.
     * Floor of a negative quotient is the negation of that ceiling. */
    round = ((uint32_t)1 << shift) - 1;
    quotient = (magnitude + round) >> shift;
    return (int32_t)((uint32_t)0 - quotient);
}
