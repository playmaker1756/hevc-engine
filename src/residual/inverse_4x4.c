#include "hevc/residual/inverse.h"

#include "hevc/common/integer_spec.h"

#include <stddef.h>
#include <stdint.h>

/* levelScale[qp % 6]. The step size is this table plus a shift of qp/6,
 * not a floating power of two. */
static const int32_t k_level_scale[6] = {40, 45, 51, 57, 64, 72};

/* 4x4 inverse sine matrix. Intra luma of size 4 uses this instead of the
 * cosine. Row i, column j. */
static const int32_t k_dst4[4][4] = {
    {29, 55, 74, 84},
    {74, 74, 0, -74},
    {84, -29, -74, 55},
    {55, -84, 74, -29},
};

/* Scale one stored level up to a transform coefficient. */
HevcStatus hevc_inv_scale_level(int32_t level,
                                int32_t qp,
                                int32_t bit_depth,
                                int32_t log2_n,
                                int32_t scale_weight,
                                int32_t *coeff)
{
    int32_t bd_shift;
    int32_t scale;
    int32_t qp_div;
    int64_t acc;
    int32_t value;

    if (coeff == NULL) {
        return HEVC_ERR_ARG;
    }
    if (qp < 0 || qp > 51 || bit_depth < 8 || bit_depth > 16 || log2_n < 2 || log2_n > 5) {
        return HEVC_ERR_RANGE;
    }
    if (scale_weight <= 0) {
        return HEVC_ERR_RANGE;
    }

    /* BitDepth + Log2(n) + 10 - 15. Extended precision is off, so the
     * transform range is 15 bits. */
    bd_shift = bit_depth + log2_n + 10 - 15;
    scale = k_level_scale[qp % 6];
    qp_div = qp / 6;
    /* The multiply is wider than int32. The stored result is clipped back
     * to the 16-bit coefficient range. */
    acc = (int64_t)level * (int64_t)scale_weight * (int64_t)scale;
    acc = acc << qp_div;
    acc = acc + ((int64_t)1 << (bd_shift - 1));
    if (acc >= 0) {
        value = (int32_t)(acc >> bd_shift);
    } else {
        /* Floor division of a negative accumulator. The magnitude fits in
         * int32 after the rounding add for the levels this clause stores. */
        value = hevc_floor_div_pow2((int32_t)acc, bd_shift);
    }
    *coeff = hevc_clip3(-32768, 32767, value);
    return HEVC_OK;
}

/* One inverse sine pass. Reads 4 coefficients and writes 4 results.
 * Each output is clipped after a rounding divide by 2. */
static void dst4_column(const int32_t *in, int32_t stride, int32_t *out)
{
    int32_t i;
    int32_t j;

    for (i = 0; i < 4; i++) {
        int32_t sum;

        sum = 0;
        for (j = 0; j < 4; j++) {
            sum = sum + k_dst4[i][j] * in[j * stride];
        }
        /* (sum + 1) >> 1. sum may be negative, so the shift is floor division. */
        out[i] = hevc_clip3(-32768, 32767, hevc_floor_div_pow2(sum + 1, 1));
    }
}

/* Inverse-scale and inverse-transform one 4x4 intra-luma block. */
HevcStatus hevc_inv_residual_4x4(const int32_t *levels,
                                 int32_t qp,
                                 int32_t bit_depth,
                                 int32_t tr_type,
                                 int32_t scale_weight,
                                 int32_t *residual)
{
    int32_t d[16];
    int32_t col[4];
    int32_t mid[16];
    int32_t row_in[4];
    int32_t row_out[4];
    int32_t i;
    int32_t x;
    int32_t y;
    int32_t final_shift;
    HevcStatus status;

    if (levels == NULL || residual == NULL) {
        return HEVC_ERR_ARG;
    }
    /* tr_type 1 is the 4x4 intra luma sine. The cosine path is not in this file. */
    if (tr_type != 1) {
        return HEVC_ERR_RANGE;
    }

    for (i = 0; i < 16; i++) {
        status = hevc_inv_scale_level(levels[i], qp, bit_depth, 2, scale_weight, &d[i]);
        if (status != HEVC_OK) {
            return status;
        }
    }

    /* Columns first. x is fixed, y runs down the column. */
    for (x = 0; x < 4; x++) {
        int32_t column_in[4];

        for (y = 0; y < 4; y++) {
            column_in[y] = d[y * 4 + x];
        }
        dst4_column(column_in, 1, col);
        for (y = 0; y < 4; y++) {
            /* Equation 8-314. The +64 rounds the divide by 128 that sits
             * between the two 1-D passes. */
            mid[y * 4 + x] = hevc_clip3(-32768, 32767, hevc_floor_div_pow2(col[y] + 64, 7));
        }
    }

    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            row_in[x] = mid[y * 4 + x];
        }
        dst4_column(row_in, 1, row_out);
        for (x = 0; x < 4; x++) {
            mid[y * 4 + x] = row_out[x];
        }
    }

    /* Equation 8-299. At 8 bits the shift is max(20 - 8, 0) = 12, and the
     * added term is 2^11. */
    final_shift = 20 - bit_depth;
    if (final_shift < 0) {
        final_shift = 0;
    }
    for (i = 0; i < 16; i++) {
        int32_t rounded;

        rounded = mid[i] + (1 << (final_shift - 1));
        residual[i] = hevc_floor_div_pow2(rounded, final_shift);
    }
    return HEVC_OK;
}
