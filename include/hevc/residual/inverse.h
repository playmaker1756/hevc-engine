#ifndef HEVC_RESIDUAL_INVERSE_H
#define HEVC_RESIDUAL_INVERSE_H

#include "hevc/status.h"

#include <stdint.h>

/* Inverse scaling and the 4x4 intra-luma inverse sine transform.
 * Clause 8.6, for the path version 0 has checked: flat scaling weight,
 * no transform skip, no extended precision.
 * coeff and residual are indexed [y * n + x].
 * A size other than 4, or a transform type other than the intra-luma sine,
 * returns HEVC_ERR_RANGE. The cosine matrix is the unread half of 8.6.4.
 */

/* Equation 8-309 for one stored level. scale_weight is 16 when scaling
 * lists are off. The product is accumulated in 64 bits because it does not
 * fit in the 16-bit coefficient, then clipped to -32768..32767.
 */
HevcStatus hevc_inv_scale_level(int32_t level,
                                int32_t qp,
                                int32_t bit_depth,
                                int32_t log2_n,
                                int32_t scale_weight,
                                int32_t *coeff);

/* Scale a block, run the inverse 4x4 sine transform, and apply the final
 * right shift of clause 8.6.2. tr_type 1 selects that sine transform.
 */
HevcStatus hevc_inv_residual_4x4(const int32_t *levels,
                                 int32_t qp,
                                 int32_t bit_depth,
                                 int32_t tr_type,
                                 int32_t scale_weight,
                                 int32_t *residual);

/* 32-point inverse cosine. Row (i << (5 - log2(n))) and columns 0..n-1
 * are the basis for a block of size n. Clause 8.6.4, equations 8-318..8-321.
 */
extern const int32_t hevc_trans_matrix[32][32];

/* Scale a square block and invert it.
 * transform_skip 1 uses equation 8-298: the scaled coefficient is shifted by
 * 5 + log2(n), then the same final shift as equation 8-299. Rotation stays
 * off because the profile forces transform_skip_rotation_enabled_flag to 0.
 * tr_type 1 is the 4x4 sine. tr_type 0 is the cosine. n is 4, 8, 16, or 32.
 */
HevcStatus hevc_inv_residual(const int32_t *levels,
                             int32_t n,
                             int32_t qp,
                             int32_t bit_depth,
                             int32_t tr_type,
                             int32_t transform_skip,
                             int32_t scale_weight,
                             int32_t *residual);

#endif
