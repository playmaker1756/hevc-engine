#include "hevc/intra/reference.h"

#include "hevc/picture/neighbor_availability.h"

#include <stddef.h>
#include <stdint.h>

/* True when n is a transform size this predictor accepts. */
static int32_t block_size_ok(int32_t n)
{
    if (n == 4 || n == 8 || n == 16 || n == 32) {
        return 1;
    }
    return 0;
}

/* Read one arm of the reference strip.
 * along_top is 1 for the row above the block and 0 for the column to the left.
 * The corner belongs to the left arm only.
 */
static HevcStatus gather_arm(const HevcPictureGeometry *g,
                             const int32_t *plane,
                             int32_t stride,
                             int32_t c_idx,
                             int32_t x0,
                             int32_t y0,
                             int32_t n,
                             int32_t same_slice,
                             int32_t same_tile,
                             int32_t along_top,
                             int32_t *samples,
                             int32_t *available)
{
    int32_t count;
    int32_t i;
    int32_t x_curr;
    int32_t y_curr;

    if (c_idx == 0) {
        x_curr = x0;
        y_curr = y0;
    } else {
        /* Chroma availability is decided in luma samples. The scale is the
         * chroma subsampling, which is 2 for 4:2:0. */
        x_curr = x0 * g->sub_width_c;
        y_curr = y0 * g->sub_height_c;
    }

    if (along_top == 1) {
        count = n * 2;
    } else {
        /* The left arm includes the corner, so it is one sample longer. */
        count = n * 2 + 1;
    }

    for (i = 0; i < count; i++) {
        int32_t x_cmp;
        int32_t y_cmp;
        int32_t x_luma;
        int32_t y_luma;
        int32_t avail;

        if (along_top == 1) {
            x_cmp = x0 + i;
            y_cmp = y0 - 1;
        } else {
            x_cmp = x0 - 1;
            /* i = 0 is y = -1, the corner. */
            y_cmp = y0 + i - 1;
        }

        if (c_idx == 0) {
            x_luma = x_cmp;
            y_luma = y_cmp;
        } else {
            x_luma = x_cmp * g->sub_width_c;
            y_luma = y_cmp * g->sub_height_c;
        }

        avail = hevc_neighbor_available(g, x_curr, y_curr, x_luma, y_luma, same_slice, same_tile);
        if (avail < 0) {
            return HEVC_ERR_RANGE;
        }
        available[i] = avail;
        if (avail == 1) {
            int64_t index;

            /* Availability is in luma samples. The plane index is still a
             * separate multiply: a wrapped y * stride would read off the
             * front of the buffer. */
            if (y_cmp < 0 || x_cmp < 0 || x_cmp >= stride) {
                return HEVC_ERR_RANGE;
            }
            index = (int64_t)y_cmp * (int64_t)stride + (int64_t)x_cmp;
            if (index > INT32_MAX) {
                return HEVC_ERR_RANGE;
            }
            samples[i] = plane[(int32_t)index];
        } else {
            /* A missing sample must not be read from the plane. Substitution
             * looks at the available flag, not at this zero. */
            samples[i] = 0;
        }
    }
    return HEVC_OK;
}

/* Fill the unfiltered intra reference strip. */
HevcStatus hevc_intra_reference_gather(const HevcPictureGeometry *g,
                                       const int32_t *plane,
                                       int32_t stride,
                                       int32_t c_idx,
                                       int32_t x0,
                                       int32_t y0,
                                       int32_t n,
                                       int32_t same_slice,
                                       int32_t same_tile,
                                       int32_t constrained_intra_pred_flag,
                                       int32_t *left,
                                       int32_t *left_available,
                                       int32_t *top,
                                       int32_t *top_available)
{
    HevcStatus status;

    if (g == NULL || plane == NULL || left == NULL || left_available == NULL ||
        top == NULL || top_available == NULL) {
        return HEVC_ERR_ARG;
    }
    if (block_size_ok(n) == 0 || stride <= 0) {
        return HEVC_ERR_RANGE;
    }
    /* The inter-neighbour gate needs each neighbour's prediction mode.
     * Version 0 sets the flag to 0, so that gate is idle. A value of 1 is
     * rejected here rather than treated as 0. */
    if (constrained_intra_pred_flag != 0) {
        return HEVC_ERR_RANGE;
    }
    if (c_idx != 0 && c_idx != 1 && c_idx != 2) {
        return HEVC_ERR_RANGE;
    }
    /* x0 + 2n and y0 + 2n stay inside a signed word. A negative origin is
     * not a block in this picture. Chroma scaling is 1 or 2. */
    if (x0 < 0 || y0 < 0 || x0 > INT32_MAX - 64 || y0 > INT32_MAX - 64) {
        return HEVC_ERR_RANGE;
    }
    if ((g->sub_width_c != 1 && g->sub_width_c != 2) ||
        (g->sub_height_c != 1 && g->sub_height_c != 2)) {
        return HEVC_ERR_RANGE;
    }

    status = gather_arm(g, plane, stride, c_idx, x0, y0, n, same_slice, same_tile,
                        0, left, left_available);
    if (status != HEVC_OK) {
        return status;
    }
    return gather_arm(g, plane, stride, c_idx, x0, y0, n, same_slice, same_tile,
                      1, top, top_available);
}
