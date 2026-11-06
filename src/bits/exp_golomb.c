#include "hevc/bits/exp_golomb.h"

#include <stddef.h>
#include <stdint.h>

/* Read one bit. The shift distance is 7..0 inside the current byte, so the
 * shifted word is non-negative. */
static HevcStatus read_one(HevcBitCursor *cursor, int32_t *bit)
{
    int32_t byte_index;
    int32_t shift;
    uint8_t byte;

    /* bit_pos is a signed index. A buffer longer than 2^28 bytes cannot
     * be addressed that way, and bit_pos + 1 must not wrap. */
    if (cursor->size_bytes < 0 || cursor->size_bytes > (INT32_MAX / 8)) {
        return HEVC_ERR_RANGE;
    }
    if (cursor->bit_pos < 0 || cursor->bit_pos >= cursor->size_bytes * 8) {
        return HEVC_ERR_RANGE;
    }
    byte_index = cursor->bit_pos >> 3;
    shift = 7 - (cursor->bit_pos & 7);
    byte = cursor->bytes[byte_index];
    *bit = (int32_t)((byte >> shift) & 1);
    cursor->bit_pos = cursor->bit_pos + 1;
    return HEVC_OK;
}

/* Read a fixed-width unsigned field. */
HevcStatus hevc_bit_read_u(HevcBitCursor *cursor, int32_t n, int32_t *value)
{
    int32_t acc;
    int32_t i;
    HevcStatus status;

    if (cursor == NULL || cursor->bytes == NULL || value == NULL) {
        return HEVC_ERR_ARG;
    }
    if (n < 0 || n > 31) {
        return HEVC_ERR_RANGE;
    }
    acc = 0;
    for (i = 0; i < n; i++) {
        int32_t bit;

        status = read_one(cursor, &bit);
        if (status != HEVC_OK) {
            return status;
        }
        /* The next bit is more significant than the bits already stored. */
        acc = (acc << 1) | bit;
    }
    *value = acc;
    return HEVC_OK;
}

/* Parse one ue(v) or the codeNum that se(v) maps. */
static HevcStatus read_code_num(HevcBitCursor *cursor, int32_t *code_num)
{
    int32_t leading;
    int32_t bit;
    int32_t suffix;
    HevcStatus status;

    /* Count zeros until the first one, and include that one in the count
     * by stopping when it arrives. leading is the number of zeros. */
    leading = 0;
    bit = 0;
    while (bit == 0) {
        status = read_one(cursor, &bit);
        if (status != HEVC_OK) {
            return status;
        }
        if (bit == 0) {
            leading = leading + 1;
            /* 2^leading must fit in a signed 32-bit word. */
            if (leading > 30) {
                return HEVC_ERR_RANGE;
            }
        }
    }
    status = hevc_bit_read_u(cursor, leading, &suffix);
    if (status != HEVC_OK) {
        return status;
    }
    /* 2^leading - 1 + suffix. The shift is non-negative and at most 31. */
    *code_num = ((1 << leading) - 1) + suffix;
    return HEVC_OK;
}

/* Unsigned Exp-Golomb. The syntax element is the code number itself. */
HevcStatus hevc_bit_read_ue(HevcBitCursor *cursor, int32_t *value)
{
    if (cursor == NULL || cursor->bytes == NULL || value == NULL) {
        return HEVC_ERR_ARG;
    }
    return read_code_num(cursor, value);
}

/* Signed Exp-Golomb. Positive magnitudes use the even code numbers. */
HevcStatus hevc_bit_read_se(HevcBitCursor *cursor, int32_t *value)
{
    int32_t code_num;
    HevcStatus status;

    if (cursor == NULL || cursor->bytes == NULL || value == NULL) {
        return HEVC_ERR_ARG;
    }
    status = read_code_num(cursor, &code_num);
    if (status != HEVC_OK) {
        return status;
    }
    if ((code_num & 1) == 0) {
        *value = -(code_num >> 1);
    } else {
        *value = (code_num + 1) >> 1;
    }
    return HEVC_OK;
}
