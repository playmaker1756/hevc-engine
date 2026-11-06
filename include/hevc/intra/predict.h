#ifndef HEVC_INTRA_PREDICT_H
#define HEVC_INTRA_PREDICT_H

#include "hevc/status.h"

#include <stdint.h>

/* Predictors for one square block. pred[y * n + x] is sample (x, y).
 * The reference strip layout is the one documented in reference.h.
 * The caller has already substituted missing samples and, when clause
 * 8.4.4.2.3 says so, smoothed the strip. DC and the angular edge rewrite
 * read that strip as given.
 */

/* Clause 8.4.4.2.5. Fill the block with the DC prediction.
 * c_idx 0 and n below 32 also filter the top row and the left column,
 * unless disable_boundary_filter is 1. Chroma and a 32-sample luma block
 * are the flat DC value.
 */
HevcStatus hevc_intra_predict_dc(int32_t n,
                                 int32_t c_idx,
                                 int32_t bit_depth,
                                 int32_t disable_boundary_filter,
                                 const int32_t *left,
                                 const int32_t *top,
                                 int32_t *pred);

/* Clause 8.4.4.2.4. Blend the left column with the top extension sample
 * p[n][-1], and the top row with the left extension sample p[-1][n].
 */
HevcStatus hevc_intra_predict_planar(int32_t n,
                                     const int32_t *left,
                                     const int32_t *top,
                                     int32_t *pred);

/* Clause 8.4.4.2.6. Project modes 2..34.
 * Modes 10 and 26 then rewrite one edge when the block is luma, smaller
 * than 32, and disable_boundary_filter is 0.
 */
HevcStatus hevc_intra_predict_angular(int32_t n,
                                      int32_t c_idx,
                                      int32_t bit_depth,
                                      int32_t mode,
                                      int32_t disable_boundary_filter,
                                      const int32_t *left,
                                      const int32_t *top,
                                      int32_t *pred);

#endif
