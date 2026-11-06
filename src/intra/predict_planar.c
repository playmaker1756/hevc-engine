#include "hevc/intra/predict.h"

#include <stdint.h>

/* Blend each sample from the two reference arms. */
HevcStatus hevc_intra_predict_planar(int32_t n,
                                     const int32_t *left,
                                     const int32_t *top,
                                     int32_t *pred)
{
    int32_t shift;
    int32_t size;
    int32_t x;
    int32_t y;
    int32_t right_ref;
    int32_t bottom_ref;

    if (left == NULL || top == NULL || pred == NULL) {
        return HEVC_ERR_ARG;
    }
    if (n != 4 && n != 8 && n != 16 && n != 32) {
        return HEVC_ERR_RANGE;
    }

    /* p[n][-1] is one step into the top extension. p[-1][n] is one step
     * into the left extension. left[n + 1] is y = n. */
    right_ref = top[n];
    bottom_ref = left[n + 1];

    shift = 0;
    size = n;
    while (size > 1) {
        shift = shift + 1;
        size = size >> 1;
    }
    /* Divide by 2n. The added n in the numerator rounds that division. */
    shift = shift + 1;

    for (y = 0; y < n; y++) {
        for (x = 0; x < n; x++) {
            int64_t horizontal;
            int64_t vertical;
            int64_t value;

            /* Each pair of weights sums to n. The products are widened so
             * two large reference samples cannot wrap the sum. */
            horizontal = (int64_t)(n - 1 - x) * (int64_t)left[y + 1] + (int64_t)(x + 1) * (int64_t)right_ref;
            vertical = (int64_t)(n - 1 - y) * (int64_t)top[x] + (int64_t)(y + 1) * (int64_t)bottom_ref;
            value = (horizontal + vertical + n) >> shift;
            if (value > 2147483647 || value < -2147483647 - 1) {
                return HEVC_ERR_RANGE;
            }
            pred[y * n + x] = (int32_t)value;
        }
    }
    return HEVC_OK;
}
