#include "hevc/intra/reference.h"

#include <stddef.h>

/* Replace missing reference samples by walking away from the far left. */
HevcStatus hevc_intra_reference_substitute(int32_t n,
                                           int32_t bit_depth,
                                           int32_t *left,
                                           int32_t *left_available,
                                           int32_t *top,
                                           int32_t *top_available)
{
    int32_t arm;
    int32_t i;
    int32_t any_present;
    int32_t mid;

    if (left == NULL || left_available == NULL || top == NULL || top_available == NULL) {
        return HEVC_ERR_ARG;
    }
    if ((n != 4 && n != 8 && n != 16 && n != 32) || bit_depth < 8 || bit_depth > 16) {
        return HEVC_ERR_RANGE;
    }

    /* 2n is the top-arm length. The left arm is one longer because of the corner. */
    arm = n * 2;
    any_present = 0;
    for (i = 0; i < arm + 1; i++) {
        if (left_available[i] == 1) {
            any_present = 1;
        }
    }
    for (i = 0; i < arm; i++) {
        if (top_available[i] == 1) {
            any_present = 1;
        }
    }

    if (any_present == 0) {
        /* No reconstructed neighbour exists. The neutral sample is the
         * midpoint of the bit depth. The shift is non-negative. */
        mid = 1 << (bit_depth - 1);
        for (i = 0; i < arm + 1; i++) {
            left[i] = mid;
            left_available[i] = 1;
        }
        for (i = 0; i < arm; i++) {
            top[i] = mid;
            top_available[i] = 1;
        }
        return HEVC_OK;
    }

    /* If the far-left sample is missing, copy only that one slot from the
     * first present sample toward the corner and then across the top. */
    if (left_available[arm] == 0) {
        int32_t found;

        found = 0;
        for (i = arm - 1; i >= 0; i--) {
            if (left_available[i] == 1) {
                left[arm] = left[i];
                found = 1;
                break;
            }
        }
        if (found == 0) {
            for (i = 0; i < arm; i++) {
                if (top_available[i] == 1) {
                    left[arm] = top[i];
                    found = 1;
                    break;
                }
            }
        }
        left_available[arm] = 1;
    }

    /* Walk up the left arm. A hole copies the sample below it, which this
     * loop has already filled. */
    for (i = arm - 1; i >= 0; i--) {
        if (left_available[i] == 0) {
            left[i] = left[i + 1];
            left_available[i] = 1;
        }
    }

    /* Walk across the top. The left neighbour of x = 0 is the corner. */
    for (i = 0; i < arm; i++) {
        if (top_available[i] == 0) {
            if (i == 0) {
                top[i] = left[0];
            } else {
                top[i] = top[i - 1];
            }
            top_available[i] = 1;
        }
    }
    return HEVC_OK;
}
