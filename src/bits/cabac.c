#include "hevc/bits/cabac.h"

#include "hevc/common/integer_spec.h"

#include <stddef.h>

/* Table 9-52. rangeTabLps[pStateIdx][qRangeIdx]. The four columns are the
 * quantized range. State 63 is the equiprobable floor of 2. */
static const int32_t k_range_tab_lps[64][4] = {
    {128, 176, 208, 240}, {128, 167, 197, 227}, {128, 158, 187, 216}, {123, 150, 178, 205},
    {116, 142, 169, 195}, {111, 135, 160, 185}, {105, 128, 152, 175}, {100, 122, 144, 166},
    {95, 116, 137, 158}, {90, 110, 130, 150}, {85, 104, 123, 142}, {81, 99, 117, 135},
    {77, 94, 111, 128}, {73, 89, 105, 122}, {69, 85, 100, 116}, {66, 80, 95, 110},
    {62, 76, 90, 104}, {59, 72, 86, 99}, {56, 69, 81, 94}, {53, 65, 77, 89},
    {51, 62, 73, 85}, {48, 59, 69, 80}, {46, 56, 66, 76}, {43, 53, 63, 72},
    {41, 50, 59, 69}, {39, 48, 56, 65}, {37, 45, 54, 62}, {35, 43, 51, 59},
    {33, 41, 48, 56}, {32, 39, 46, 53}, {30, 37, 43, 50}, {29, 35, 41, 48},
    {27, 33, 39, 45}, {26, 31, 37, 43}, {24, 30, 35, 41}, {23, 28, 33, 39},
    {22, 27, 32, 37}, {21, 26, 30, 35}, {20, 24, 29, 33}, {19, 23, 27, 31},
    {18, 22, 26, 30}, {17, 21, 25, 28}, {16, 20, 23, 27}, {15, 19, 22, 25},
    {14, 18, 21, 24}, {14, 17, 20, 23}, {13, 16, 19, 22}, {12, 15, 18, 21},
    {12, 14, 17, 20}, {11, 14, 16, 19}, {11, 13, 15, 18}, {10, 12, 15, 17},
    {10, 12, 14, 16}, {9, 11, 13, 15}, {9, 11, 12, 14}, {8, 10, 12, 14},
    {8, 9, 11, 13}, {7, 9, 11, 12}, {7, 9, 10, 12}, {7, 8, 10, 11},
    {6, 8, 9, 11}, {6, 7, 9, 10}, {6, 7, 8, 9}, {2, 2, 2, 2},
};

/* Table 9-53. Next state after a most probable symbol and after a least probable one. */
static const int32_t k_trans_mps[64] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 62, 63
};

static const int32_t k_trans_lps[64] = {
    0, 0, 1, 2, 2, 4, 4, 5, 6, 7, 8, 9, 9, 11, 11, 12,
    13, 13, 15, 15, 16, 16, 18, 18, 19, 19, 21, 21, 22, 22, 23, 24,
    24, 25, 26, 26, 27, 27, 28, 29, 29, 30, 30, 30, 31, 32, 32, 33,
    33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 63
};

/* The engine keeps offset below range, and range in 2..510.
 * A broken pair is rejected before a shift can walk off the word. */
static HevcStatus engine_in_range(const HevcCabacEngine *engine)
{
    if (engine->range < 2 || engine->range > 510) {
        return HEVC_ERR_RANGE;
    }
    if (engine->offset < 0 || engine->offset >= engine->range) {
        return HEVC_ERR_RANGE;
    }
    return HEVC_OK;
}

/* Double the range and shift one bit into the offset until the range reaches 256. */
static HevcStatus renorm(HevcCabacEngine *engine)
{
    while (engine->range < 256) {
        int32_t bit;
        HevcStatus status;

        engine->range = engine->range << 1;
        status = hevc_bit_read_u(engine->cursor, 1, &bit);
        if (status != HEVC_OK) {
            return status;
        }
        engine->offset = (engine->offset << 1) | bit;
    }
    return HEVC_OK;
}

/* Start the engine at range 510 and take the first nine offset bits. */
HevcStatus hevc_cabac_engine_init(HevcCabacEngine *engine, HevcBitCursor *cursor)
{
    int32_t offset;
    HevcStatus status;

    if (engine == NULL || cursor == NULL) {
        return HEVC_ERR_ARG;
    }
    status = hevc_bit_read_u(cursor, 9, &offset);
    if (status != HEVC_OK) {
        return status;
    }
    /* 510 and 511 are forbidden offset values at initialization. */
    if (offset >= 510) {
        return HEVC_ERR_RANGE;
    }
    engine->range = 510;
    engine->offset = offset;
    engine->cursor = cursor;
    return HEVC_OK;
}

/* Turn one initValue and the slice QP into a context state. */
HevcStatus hevc_cabac_context_init(int32_t init_value, int32_t slice_qp, HevcCabacContext *ctx)
{
    int32_t slope;
    int32_t offset_idx;
    int32_t m;
    int32_t n;
    int32_t qp;
    int32_t pre;
    int32_t product;

    if (ctx == NULL) {
        return HEVC_ERR_ARG;
    }
    if (init_value < 0 || init_value > 255) {
        return HEVC_ERR_RANGE;
    }
    slope = init_value >> 4;
    offset_idx = init_value & 15;
    m = slope * 5 - 45;
    n = (offset_idx << 3) - 16;
    qp = hevc_clip3(0, 51, slice_qp);
    /* (m * qp) >> 4. m may be negative, so the shift is floor division. */
    product = hevc_floor_div_pow2(m * qp, 4);
    pre = hevc_clip3(1, 126, product + n);
    if (pre <= 63) {
        ctx->val_mps = 0;
        ctx->p_state = 63 - pre;
    } else {
        ctx->val_mps = 1;
        ctx->p_state = pre - 64;
    }
    return HEVC_OK;
}

/* initType from the slice type and cabac_init_flag. I is 2 in the syntax. */
int32_t hevc_cabac_init_type(int32_t slice_type, int32_t cabac_init_flag)
{
    if (slice_type == 2) {
        return 0;
    }
    if (slice_type == 1) {
        if (cabac_init_flag == 0) {
            return 1;
        }
        return 2;
    }
    if (slice_type == 0) {
        if (cabac_init_flag == 0) {
            return 2;
        }
        return 1;
    }
    return -1;
}

/* Decode one context bin. */
HevcStatus hevc_cabac_decision(HevcCabacEngine *engine, HevcCabacContext *ctx, int32_t *bin)
{
    int32_t q_range;
    int32_t lps_range;
    int32_t bin_val;

    if (engine == NULL || engine->cursor == NULL || ctx == NULL || bin == NULL) {
        return HEVC_ERR_ARG;
    }
    if (engine_in_range(engine) != HEVC_OK) {
        return HEVC_ERR_RANGE;
    }
    if (ctx->p_state < 0 || ctx->p_state > 63) {
        return HEVC_ERR_RANGE;
    }

    q_range = (engine->range >> 6) & 3;
    lps_range = k_range_tab_lps[ctx->p_state][q_range];
    engine->range = engine->range - lps_range;
    if (engine->offset >= engine->range) {
        bin_val = 1 - ctx->val_mps;
        engine->offset = engine->offset - engine->range;
        engine->range = lps_range;
    } else {
        bin_val = ctx->val_mps;
    }

    if (bin_val == ctx->val_mps) {
        ctx->p_state = k_trans_mps[ctx->p_state];
    } else {
        if (ctx->p_state == 0) {
            ctx->val_mps = 1 - ctx->val_mps;
        }
        ctx->p_state = k_trans_lps[ctx->p_state];
    }
    *bin = bin_val;
    return renorm(engine);
}

/* Decode one equiprobable bin. */
HevcStatus hevc_cabac_bypass(HevcCabacEngine *engine, int32_t *bin)
{
    int32_t bit;
    HevcStatus status;

    if (engine == NULL || engine->cursor == NULL || bin == NULL) {
        return HEVC_ERR_ARG;
    }
    if (engine_in_range(engine) != HEVC_OK) {
        return HEVC_ERR_RANGE;
    }
    status = hevc_bit_read_u(engine->cursor, 1, &bit);
    if (status != HEVC_OK) {
        return status;
    }
    engine->offset = (engine->offset << 1) | bit;
    if (engine->offset >= engine->range) {
        *bin = 1;
        engine->offset = engine->offset - engine->range;
    } else {
        *bin = 0;
    }
    return HEVC_OK;
}

/* Decode the bin that ends a slice or a subset. */
HevcStatus hevc_cabac_terminate(HevcCabacEngine *engine, int32_t *bin, int32_t *finished)
{
    if (engine == NULL || engine->cursor == NULL || bin == NULL || finished == NULL) {
        return HEVC_ERR_ARG;
    }
    if (engine_in_range(engine) != HEVC_OK) {
        return HEVC_ERR_RANGE;
    }
    engine->range = engine->range - 2;
    if (engine->offset >= engine->range) {
        *bin = 1;
        *finished = 1;
        return HEVC_OK;
    }
    *bin = 0;
    *finished = 0;
    return renorm(engine);
}
