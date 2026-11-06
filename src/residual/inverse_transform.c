#include "hevc/residual/inverse.h"

#include "hevc/common/integer_spec.h"

#include <stddef.h>
#include <stdint.h>

/* log2 of a power of two in 4..32. Returns -1 for any other size. */
static int32_t log2_size(int32_t n)
{
    int32_t log2;
    int32_t size;

    log2 = 0;
    size = n;
    while (size > 1) {
        if ((size & 1) != 0) {
            return -1;
        }
        size = size >> 1;
        log2 = log2 + 1;
    }
    if (log2 < 2 || log2 > 5) {
        return -1;
    }
    return log2;
}

/* One inverse cosine pass. Row i of the block reads matrix row i << shift. */
static void cosine_1d(const int32_t *in, int32_t n, int32_t shift, int32_t *out)
{
    int32_t i;
    int32_t j;

    for (i = 0; i < n; i++) {
        int32_t sum;
        int32_t row;

        sum = 0;
        row = i << shift;
        for (j = 0; j < n; j++) {
            sum = sum + hevc_trans_matrix[row][j] * in[j];
        }
        out[i] = hevc_clip3(-32768, 32767, sum);
    }
}

/* One inverse sine pass. The 4x4 matrix is the one already used by the
 * 4x4 entry point. It is duplicated here so this file can invert a skip
 * or a cosine block without calling back into that entry point. */
static const int32_t k_dst4[4][4] = {
    {29, 55, 74, 84},
    {74, 74, 0, -74},
    {84, -29, -74, 55},
    {55, -84, 74, -29},
};

static void sine_1d(const int32_t *in, int32_t *out)
{
    int32_t i;
    int32_t j;

    for (i = 0; i < 4; i++) {
        int32_t sum;

        sum = 0;
        for (j = 0; j < 4; j++) {
            sum = sum + k_dst4[i][j] * in[j];
        }
        out[i] = hevc_clip3(-32768, 32767, hevc_floor_div_pow2(sum + 1, 1));
    }
}

/* Inverse-scale, then sine, cosine, or transform skip, then the final shift. */
HevcStatus hevc_inv_residual(const int32_t *levels,
                             int32_t n,
                             int32_t qp,
                             int32_t bit_depth,
                             int32_t tr_type,
                             int32_t transform_skip,
                             int32_t scale_weight,
                             int32_t *residual)
{
    int32_t log2_n;
    int32_t count;
    int32_t i;
    int32_t x;
    int32_t y;
    int32_t shift;
    int32_t ts_shift;
    int32_t final_shift;
    int32_t d[32 * 32];
    int32_t mid[32 * 32];
    int32_t line_in[32];
    int32_t line_out[32];
    HevcStatus status;

    if (levels == NULL || residual == NULL) {
        return HEVC_ERR_ARG;
    }
    log2_n = log2_size(n);
    if (log2_n < 0) {
        return HEVC_ERR_RANGE;
    }
    if (tr_type == 1 && n != 4) {
        return HEVC_ERR_RANGE;
    }
    if (tr_type != 0 && tr_type != 1) {
        return HEVC_ERR_RANGE;
    }

    count = n * n;
    for (i = 0; i < count; i++) {
        status = hevc_inv_scale_level(levels[i], qp, bit_depth, log2_n, scale_weight, &d[i]);
        if (status != HEVC_OK) {
            return status;
        }
    }

    /* Equation 8-295. Extended precision is off, so the floor is 0.
     * For 8-bit to 16-bit samples, 20 - bit depth is already positive. */
    final_shift = 20 - bit_depth;
    if (final_shift < 0) {
        final_shift = 0;
    }

    if (transform_skip == 1) {
        /* Equation 8-296. The profile forces rotation off, so d[x][y] is
         * not mirrored before the shift. The shift is applied to the
         * magnitude: a negative coefficient is not shifted as a signed value. */
        ts_shift = 5 + log2_n;
        for (i = 0; i < count; i++) {
            if (d[i] >= 0) {
                mid[i] = d[i] << ts_shift;
            } else {
                uint32_t magnitude;

                magnitude = 0u - (uint32_t)d[i];
                magnitude = magnitude << (uint32_t)ts_shift;
                mid[i] = 0 - (int32_t)magnitude;
            }
        }
    } else if (tr_type == 1) {
        for (x = 0; x < 4; x++) {
            for (y = 0; y < 4; y++) {
                line_in[y] = d[y * 4 + x];
            }
            sine_1d(line_in, line_out);
            for (y = 0; y < 4; y++) {
                mid[y * 4 + x] = hevc_clip3(-32768, 32767, hevc_floor_div_pow2(line_out[y] + 64, 7));
            }
        }
        for (y = 0; y < 4; y++) {
            for (x = 0; x < 4; x++) {
                line_in[x] = mid[y * 4 + x];
            }
            sine_1d(line_in, line_out);
            for (x = 0; x < 4; x++) {
                mid[y * 4 + x] = line_out[x];
            }
        }
    } else {
        /* 5 - log2(n) selects every (32/n)th matrix row. Columns stay contiguous. */
        shift = 5 - log2_n;
        for (x = 0; x < n; x++) {
            for (y = 0; y < n; y++) {
                line_in[y] = d[y * n + x];
            }
            cosine_1d(line_in, n, shift, line_out);
            for (y = 0; y < n; y++) {
                mid[y * n + x] = hevc_clip3(-32768, 32767, hevc_floor_div_pow2(line_out[y] + 64, 7));
            }
        }
        for (y = 0; y < n; y++) {
            for (x = 0; x < n; x++) {
                line_in[x] = mid[y * n + x];
            }
            cosine_1d(line_in, n, shift, line_out);
            for (x = 0; x < n; x++) {
                mid[y * n + x] = line_out[x];
            }
        }
    }

    for (i = 0; i < count; i++) {
        if (final_shift == 0) {
            residual[i] = mid[i];
        } else {
            residual[i] = hevc_floor_div_pow2(mid[i] + (1 << (final_shift - 1)), final_shift);
        }
    }
    return HEVC_OK;
}
