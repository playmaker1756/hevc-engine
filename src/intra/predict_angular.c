#include "hevc/intra/predict.h"

#include "hevc/common/integer_spec.h"

/* Table 8-5. IntraPredAngle in units of 1/32 sample, indexed by mode.
 * Modes 0 and 1 are planar and DC and are not read.
 * The values are the specification table. They are not recomputed.
 */
static const int32_t k_angle[35] = {
    0, 0,
    32, 26, 21, 17, 13, 9, 5, 2, 0,
    -2, -5, -9, -13, -17, -21, -26,
    -32, -26, -21, -17, -13, -9, -5, -2,
    0, 2, 5, 9, 13, 17, 21, 26, 32
};

/* Table 8-6. Inverse angle for modes 11..25, in mode order.
 * Mode 12 is -1638 because the specification truncates 8192/5.
 * Rounding that division would store -1639, so this table is not derived
 * again at runtime.
 */
static const int32_t k_inv_angle[15] = {
    -4096, -1638, -910, -630, -482, -390, -256,
    -256, -315, -390, -482, -630, -910, -1638, -4096
};

/* Lowest reference index this file stores. A 32-sample block at angle -32
 * reaches index -32. */
static const int32_t k_ref_bias = 32;

static int32_t clip_sample(int32_t value, int32_t bit_depth)
{
    int32_t max_sample;

    max_sample = (1 << bit_depth) - 1;
    return hevc_clip3(0, max_sample, value);
}

/* The stored reference covers indices -32..64. Anything else is a bug
 * in the projection, not a sample that may be read off the array. */
static int32_t ref_index_ok(int32_t index)
{
    if (index < -k_ref_bias || index > 64) {
        return 0;
    }
    return 1;
}

/* ref_at reads the biased reference array. index may be negative. */
static HevcStatus ref_at(const int32_t *ref, int32_t index, int32_t *sample)
{
    if (ref_index_ok(index) == 0) {
        return HEVC_ERR_RANGE;
    }
    *sample = ref[index + k_ref_bias];
    return HEVC_OK;
}

static HevcStatus ref_put(int32_t *ref, int32_t index, int32_t sample)
{
    if (ref_index_ok(index) == 0) {
        return HEVC_ERR_RANGE;
    }
    ref[index + k_ref_bias] = sample;
    return HEVC_OK;
}

/* Build the 1/32-sample reference for a projection that starts on the main
 * arm and, when the angle is negative, borrows the other arm.
 * main_is_top is 1 when mode >= 18. The main arm then supplies positive
 * indices and the left arm supplies negative indices.
 */
static HevcStatus build_ref(int32_t n,
                             int32_t angle,
                             int32_t inv_angle,
                             int32_t main_is_top,
                             const int32_t *left,
                             const int32_t *top,
                             int32_t *ref)
{
    int32_t x;
    int32_t last;
    HevcStatus status;

    /* ref[0] is the corner. ref[1] .. ref[n] are the n samples beside the block. */
    status = ref_put(ref, 0, left[0]);
    if (status != HEVC_OK) {
        return status;
    }
    for (x = 1; x <= n; x++) {
        if (main_is_top == 1) {
            status = ref_put(ref, x, top[x - 1]);
        } else {
            status = ref_put(ref, x, left[x]);
        }
        if (status != HEVC_OK) {
            return status;
        }
    }

    if (angle >= 0) {
        /* The extension continues along the same arm out to 2n. */
        for (x = n + 1; x <= n * 2; x++) {
            if (main_is_top == 1) {
                status = ref_put(ref, x, top[x - 1]);
            } else {
                /* left[x] is y = x - 1, which is p[-1][-1 + x]. */
                status = ref_put(ref, x, left[x]);
            }
            if (status != HEVC_OK) {
                return status;
            }
        }
        return HEVC_OK;
    }

    last = hevc_floor_div_pow2(n * angle, 5);
    /* The extension is specified only when it reaches index -2 or below.
     * Index -1 is never read: the projection adds 1 before indexing. */
    if (last < -1) {
        for (x = -1; x >= last; x--) {
            int32_t pos;
            int32_t sample_y;

            /* x and inv_angle are both negative, so the product is positive.
             * The +128 rounds the divide by 256. */
            /* Both factors are negative, so the product is positive and
             * fits: the largest pair this table forms is 32 * 4096. */
            pos = ((x * inv_angle) + 128) >> 8;
            sample_y = -1 + pos;
            /* The borrowed arm is 2n samples. An index past it is not read. */
            if (sample_y < 0 || sample_y >= n * 2) {
                return HEVC_ERR_RANGE;
            }
            if (main_is_top == 1) {
                /* Negative top indices come from the left column. */
                status = ref_put(ref, x, left[sample_y + 1]);
            } else {
                status = ref_put(ref, x, top[sample_y]);
            }
            if (status != HEVC_OK) {
                return status;
            }
        }
    }
    return HEVC_OK;
}

static HevcStatus project(int32_t n, int32_t angle, int32_t main_is_top,
                           const int32_t *ref, int32_t *pred)
{
    int32_t row;
    int32_t col;

    for (row = 0; row < n; row++) {
        for (col = 0; col < n; col++) {
            int32_t pos;
            int32_t product;
            int32_t i_idx;
            int32_t i_fact;
            int32_t base;
            int32_t sample;
            HevcStatus status;

            /* Mode >= 18 displaces by the row. Mode < 18 displaces by the column. */
            if (main_is_top == 1) {
                pos = row;
            } else {
                pos = col;
            }
            product = (pos + 1) * angle;
            /* Low 5 bits are the fraction. The rest is the whole-sample index.
             * A negative product uses floor division so the fraction stays
             * in 0..31. Toward-zero division would pick the next reference. */
            i_idx = hevc_floor_div_pow2(product, 5);
            i_fact = product - i_idx * 32;

            if (main_is_top == 1) {
                base = col + i_idx + 1;
            } else {
                base = row + i_idx + 1;
            }
            if (i_fact == 0) {
                status = ref_at(ref, base, &sample);
                if (status != HEVC_OK) {
                    return status;
                }
            } else {
                int32_t near;
                int32_t far;
                int64_t mixed;

                status = ref_at(ref, base, &near);
                if (status != HEVC_OK) {
                    return status;
                }
                status = ref_at(ref, base + 1, &far);
                if (status != HEVC_OK) {
                    return status;
                }
                /* (32 - fraction) * near + fraction * far, rounded divide by 32.
                 * The products are widened so two large samples cannot wrap. */
                mixed = (int64_t)(32 - i_fact) * (int64_t)near + (int64_t)i_fact * (int64_t)far + 16;
                sample = (int32_t)(mixed >> 5);
            }
            pred[row * n + col] = sample;
        }
    }
    return HEVC_OK;
}

/* Project modes 2..34 and rewrite the mode 10 or 26 edge when required. */
HevcStatus hevc_intra_predict_angular(int32_t n,
                                      int32_t c_idx,
                                      int32_t bit_depth,
                                      int32_t mode,
                                      int32_t disable_boundary_filter,
                                      const int32_t *left,
                                      const int32_t *top,
                                      int32_t *pred)
{
    int32_t angle;
    int32_t inv_angle;
    int32_t main_is_top;
    int32_t ref[97];
    int32_t y;
    int32_t x;
    int32_t i;
    HevcStatus status;

    if (left == NULL || top == NULL || pred == NULL) {
        return HEVC_ERR_ARG;
    }
    if ((n != 4 && n != 8 && n != 16 && n != 32) || bit_depth < 8 || bit_depth > 16) {
        return HEVC_ERR_RANGE;
    }
    if (mode < 2 || mode > 34) {
        return HEVC_ERR_RANGE;
    }

    angle = k_angle[mode];
    inv_angle = 0;
    if (mode >= 11 && mode <= 25) {
        inv_angle = k_inv_angle[mode - 11];
    }
    if (mode >= 18) {
        main_is_top = 1;
    } else {
        main_is_top = 0;
    }

    /* Unfilled slots stay 0. A missed index then predicts a defined sample
     * instead of whatever was on the stack. */
    for (i = 0; i < 97; i++) {
        ref[i] = 0;
    }
    status = build_ref(n, angle, inv_angle, main_is_top, left, top, ref);
    if (status != HEVC_OK) {
        return status;
    }
    status = project(n, angle, main_is_top, ref, pred);
    if (status != HEVC_OK) {
        return status;
    }

    /* Modes 10 and 26 did not pass through reference smoothing. The edge
     * rewrite mixes the prediction with the unsmoothed neighbour. */
    if (c_idx != 0 || n >= 32 || disable_boundary_filter != 0) {
        return HEVC_OK;
    }
    if (mode == 26) {
        for (y = 0; y < n; y++) {
            int32_t delta;

            delta = hevc_floor_div_pow2(left[y + 1] - left[0], 1);
            pred[y * n] = clip_sample(top[0] + delta, bit_depth);
        }
    }
    if (mode == 10) {
        for (x = 0; x < n; x++) {
            int32_t delta;

            delta = hevc_floor_div_pow2(top[x] - left[0], 1);
            pred[x] = clip_sample(left[1] + delta, bit_depth);
        }
    }
    return HEVC_OK;
}
