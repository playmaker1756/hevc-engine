#ifndef HEVC_BITS_EXP_GOLOMB_H
#define HEVC_BITS_EXP_GOLOMB_H

#include "hevc/status.h"

#include <stdint.h>

/* A cursor into an RBSP. Bits are read most significant bit first.
 * bit_pos is the index of the next bit. The caller owns the bytes.
 */
typedef struct HevcBitCursor {
    const uint8_t *bytes;
    int32_t size_bytes;
    int32_t bit_pos;
} HevcBitCursor;

/* Read n bits, n in 0..31, as an unsigned integer. */
HevcStatus hevc_bit_read_u(HevcBitCursor *cursor, int32_t n, int32_t *value);

/* Clause 9.2. Unsigned Exp-Golomb, ue(v). */
HevcStatus hevc_bit_read_ue(HevcBitCursor *cursor, int32_t *value);

/* Clause 9.2.2. Signed Exp-Golomb, se(v).
 * Code 0 is 0, code 1 is +1, code 2 is -1, code 3 is +2.
 */
HevcStatus hevc_bit_read_se(HevcBitCursor *cursor, int32_t *value);

#endif
