#include "hevc/loop/sao.h"

#include "hevc/common/integer_spec.h"

#include <stdint.h>

/* Sign of a - b. The subtraction is widened so the two samples cannot
 * overflow a signed 32-bit difference. */
static int32_t sign_of_diff(int32_t left, int32_t right)
{
    int64_t diff;

    diff = (int64_t)left - (int64_t)right;
    if (diff > 0) {
        return 1;
    }
    if (diff < 0) {
        return -1;
    }
    return 0;
}

/* Classify one sample against its two edge neighbours. */
int32_t hevc_sao_edge_category(int32_t center, int32_t neighbour0, int32_t neighbour1)
{
    int32_t edge;

    edge = 2 + sign_of_diff(center, neighbour0) + sign_of_diff(center, neighbour1);
    /* Categories 0, 1, and 2 are remapped. A raw 2 is a slope and becomes
     * the no-offset category. A raw 0 is a local minimum and becomes 1. */
    if (edge == 0 || edge == 1 || edge == 2) {
        if (edge == 2) {
            return 0;
        }
        return edge + 1;
    }
    return edge;
}

/* Map a sample onto one of four bands, or onto 0 when it misses them. */
int32_t hevc_sao_band_category(int32_t sample, int32_t bit_depth, int32_t left_class)
{
    int32_t shift;
    int32_t band;
    int32_t k;

    if (bit_depth < 8 || bit_depth > 16 || sample < 0) {
        return 0;
    }
    /* 8-bit samples shift by 3, which splits the range into 32 bands.
     * sample is non-negative, so the shift is a logical divide. */
    shift = bit_depth - 5;
    band = sample >> shift;
    for (k = 0; k < 4; k++) {
        if (((k + left_class) & 31) == band) {
            return k + 1;
        }
    }
    return 0;
}

/* Inferred sign bit for an edge-offset magnitude. */
int32_t hevc_sao_edge_sign(int32_t category_slot)
{
    /* Slots 0 and 1 become SaoOffsetVal[1] and [2], which lift valleys.
     * Slots 2 and 3 become SaoOffsetVal[3] and [4], which pull peaks down. */
    if (category_slot == 0 || category_slot == 1) {
        return 0;
    }
    return 1;
}

/* Add a signed offset and clip to the sample range. */
int32_t hevc_sao_apply(int32_t sample, int32_t offset, int32_t bit_depth)
{
    int32_t max_sample;
    int64_t sum;

    if (bit_depth < 8 || bit_depth > 16) {
        return 0;
    }
    /* The shift is 8..16, so the maximum sample fits in a signed word. */
    max_sample = (1 << bit_depth) - 1;
    /* sample + offset is widened. A wrapped sum would clip the wrong way. */
    sum = (int64_t)sample + (int64_t)offset;
    if (sum < 0) {
        return 0;
    }
    if (sum > max_sample) {
        return max_sample;
    }
    return (int32_t)sum;
}
