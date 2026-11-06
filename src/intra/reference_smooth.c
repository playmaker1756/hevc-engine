#include "hevc/intra/reference.h"

#include <stddef.h>

/* Copy one strip. Used when clause 8.4.4.2.3 leaves the samples unfiltered. */
static void copy_strip(int32_t n, const int32_t *left, const int32_t *top,
                       int32_t *left_out, int32_t *top_out)
{
    int32_t arm;
    int32_t i;

    arm = n * 2;
    for (i = 0; i < arm + 1; i++) {
        left_out[i] = left[i];
    }
    for (i = 0; i < arm; i++) {
        top_out[i] = top[i];
    }
}

/* Three-tap filter. Each output reads the unfiltered inputs.
 * The far end of each arm is copied unchanged. */
static void smooth_three_tap(int32_t n, const int32_t *left, const int32_t *top,
                             int32_t *left_out, int32_t *top_out)
{
    int32_t arm;
    int32_t i;

    arm = n * 2;
    /* Corner: the sample below it on the left arm and the first top sample. */
    left_out[0] = (left[1] + 2 * left[0] + top[0] + 2) >> 2;
    for (i = 1; i < arm; i++) {
        /* y runs 0..2n-2. The three taps are y+1, y, and y-1. */
        left_out[i] = (left[i + 1] + 2 * left[i] + left[i - 1] + 2) >> 2;
    }
    left_out[arm] = left[arm];

    top_out[0] = (left[0] + 2 * top[0] + top[1] + 2) >> 2;
    for (i = 1; i < arm - 1; i++) {
        top_out[i] = (top[i - 1] + 2 * top[i] + top[i + 1] + 2) >> 2;
    }
    top_out[arm - 1] = top[arm - 1];
}

/* Linear ramp. Weights (63 - t) and (t + 1) sum to 64. The +32 rounds
 * the divide by 64. The caller keeps the two end samples unchanged.
 * t is the distance from the corner along that arm.
 */
static int32_t strong_sample(int32_t t, int32_t corner, int32_t far)
{
    int32_t near_w;
    int32_t far_w;

    near_w = 63 - t;
    far_w = t + 1;
    return (near_w * corner + far_w * far + 32) >> 6;
}

/* Filter the reference strip, or copy it when the specification leaves it alone. */
HevcStatus hevc_intra_reference_smooth(int32_t n,
                                       int32_t c_idx,
                                       int32_t bit_depth,
                                       int32_t mode,
                                       int32_t strong_intra_smoothing_enabled_flag,
                                       const int32_t *left,
                                       const int32_t *top,
                                       int32_t *left_out,
                                       int32_t *top_out)
{
    int32_t dist_26;
    int32_t dist_10;
    int32_t dist;
    int32_t threshold;
    int32_t filter;

    if (left == NULL || top == NULL || left_out == NULL || top_out == NULL) {
        return HEVC_ERR_ARG;
    }
    if ((n != 4 && n != 8 && n != 16 && n != 32) || bit_depth < 8 || bit_depth > 16) {
        return HEVC_ERR_RANGE;
    }
    if (mode < 0 || mode > 34) {
        return HEVC_ERR_RANGE;
    }

    /* 4:2:0 chroma is not filtered. 4:4:4 would reach this filter; version 0
     * is 4:2:0, so any chroma index is copied. */
    filter = 1;
    if (c_idx != 0) {
        filter = 0;
    }
    if (n == 4 || mode == 1) {
        filter = 0;
    }

    if (filter == 1) {
        dist_26 = mode - 26;
        if (dist_26 < 0) {
            dist_26 = -dist_26;
        }
        dist_10 = mode - 10;
        if (dist_10 < 0) {
            dist_10 = -dist_10;
        }
        dist = dist_26;
        if (dist_10 < dist) {
            dist = dist_10;
        }
        /* Table 8-4. The filter runs only when the mode is farther from
         * both horizontal and vertical than this threshold. */
        if (n == 8) {
            threshold = 7;
        } else if (n == 16) {
            threshold = 1;
        } else {
            threshold = 0;
        }
        if (dist > threshold) {
            filter = 1;
        } else {
            filter = 0;
        }
    }

    if (filter == 0) {
        copy_strip(n, left, top, left_out, top_out);
        return HEVC_OK;
    }

    if (n == 32 && strong_intra_smoothing_enabled_flag == 1) {
        int32_t limit;
        int32_t left_bend;
        int32_t top_bend;

        /* 1 << (bit_depth - 5) is 8 at 8 bits. The arm is straight enough
         * when the midpoint lies within that limit of the straight line. */
        limit = 1 << (bit_depth - 5);
        left_bend = left[0] + left[64] - 2 * left[32];
        if (left_bend < 0) {
            left_bend = -left_bend;
        }
        top_bend = left[0] + top[63] - 2 * top[31];
        if (top_bend < 0) {
            top_bend = -top_bend;
        }
        if (left_bend < limit && top_bend < limit) {
            int32_t t;

            /* Left index 0 is the corner and index 64 is y = 63. Both stay.
             * Indices 1..63 are y = 0..62. */
            left_out[0] = left[0];
            for (t = 0; t <= 62; t++) {
                left_out[t + 1] = strong_sample(t, left[0], left[64]);
            }
            left_out[64] = left[64];
            /* Top has no separate corner slot. x = 0..62 is the ramp and
             * x = 63 stays. The corner used as the near end is left[0]. */
            for (t = 0; t <= 62; t++) {
                top_out[t] = strong_sample(t, left[0], top[63]);
            }
            top_out[63] = top[63];
            return HEVC_OK;
        }
    }

    smooth_three_tap(n, left, top, left_out, top_out);
    return HEVC_OK;
}
