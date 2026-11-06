#include "hevc/intra/predict.h"

/* Fill one block with DC, then filter the luma edge when the size allows it. */
HevcStatus hevc_intra_predict_dc(int32_t n,
                                 int32_t c_idx,
                                 int32_t bit_depth,
                                 int32_t disable_boundary_filter,
                                 const int32_t *left,
                                 const int32_t *top,
                                 int32_t *pred)
{
    int32_t sum;
    int32_t i;
    int32_t x;
    int32_t y;
    int32_t shift;
    int32_t dc;
    int32_t filter_edge;

    if (left == NULL || top == NULL || pred == NULL) {
        return HEVC_ERR_ARG;
    }
    if ((n != 4 && n != 8 && n != 16 && n != 32) || bit_depth < 8 || bit_depth > 16) {
        return HEVC_ERR_RANGE;
    }

    /* n adjacent left samples and n adjacent top samples. The corner and
     * the extension past the block are not part of the average. */
    sum = n;
    for (i = 0; i < n; i++) {
        sum = sum + left[i + 1];
        sum = sum + top[i];
    }
    /* log2(n) + 1 divides by 2n. n is a power of two, so the shift count
     * is exact. The added n above rounds that division. */
    shift = 0;
    i = n;
    while (i > 1) {
        shift = shift + 1;
        i = i >> 1;
    }
    shift = shift + 1;
    dc = sum >> shift;

    for (y = 0; y < n; y++) {
        for (x = 0; x < n; x++) {
            pred[y * n + x] = dc;
        }
    }

    /* Chroma, a 32-sample luma block, and a disabled boundary filter stay flat. */
    filter_edge = 1;
    if (c_idx != 0 || n == 32 || disable_boundary_filter != 0) {
        filter_edge = 0;
    }
    if (filter_edge == 0) {
        return HEVC_OK;
    }

    /* The DC edge filter is the shift itself. This clause does not clip it.
     * Corner uses the left sample at y = 0 and the top sample at x = 0. */
    pred[0] = (left[1] + 2 * dc + top[0] + 2) >> 2;
    for (x = 1; x < n; x++) {
        pred[x] = (top[x] + 3 * dc + 2) >> 2;
    }
    for (y = 1; y < n; y++) {
        pred[y * n] = (left[y + 1] + 3 * dc + 2) >> 2;
    }
    return HEVC_OK;
}
