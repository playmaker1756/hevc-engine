#ifndef HEVC_INTRA_REFERENCE_H
#define HEVC_INTRA_REFERENCE_H

#include "hevc/picture/geometry.h"
#include "hevc/status.h"

#include <stdint.h>

/* Reference strip for one square intra block of size n. Clause 8.4.4.2.
 * left[i] is the sample at x = -1, y = i - 1, for i = 0..2n.
 * i = 0 is the corner above the left column.
 * top[i] is the sample at y = -1, x = i, for i = 0..2n-1.
 * The corner is stored only in left[0].
 * available entries are 1 when the sample may be read and 0 when it is missing.
 * The caller provides 2n+1 left slots and 2n top slots.
 */

/* Clause 8.4.4.2.1. Read reconstructed samples into the strip.
 * c_idx 0 is luma. Any other c_idx is chroma: the sample location is scaled
 * by SubWidthC and SubHeightC before the luma availability test.
 * same_slice and same_tile are passed to every neighbour. One slice and one
 * tile pass 1 and 1. A per-sample slice map replaces those two arguments.
 * constrained_intra_pred_flag must be 0. Version 0 forces that flag off, so
 * the second gate (reject an inter neighbour) is not applied here.
 * Returns HEVC_ERR_ARG for a null pointer, HEVC_ERR_RANGE for a block this
 * function cannot describe.
 */
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
                                       int32_t *top_available);

/* Clause 8.4.4.2.2. Replace every missing sample.
 * When the whole strip is missing, every sample becomes 1 << (bit_depth - 1).
 * Otherwise a hole copies the already-visited neighbour, walking from the
 * bottom of the left arm up to the corner and then across the top.
 * Samples that were present are left unchanged. Available flags become 1.
 */
HevcStatus hevc_intra_reference_substitute(int32_t n,
                                           int32_t bit_depth,
                                           int32_t *left,
                                           int32_t *left_available,
                                           int32_t *top,
                                           int32_t *top_available);

/* Clause 8.4.4.2.3. Filter the strip when the mode and the size require it.
 * Writes left_out and top_out. When the filter is off, those arrays copy
 * the inputs. c_idx other than 0 is copied unchanged for 4:2:0.
 * strong_intra_smoothing_enabled_flag selects the linear ramp on a 32-sample
 * luma block when both arms are nearly straight. Otherwise the 3-tap filter
 * is used. DC and n = 4 are never filtered.
 */
HevcStatus hevc_intra_reference_smooth(int32_t n,
                                       int32_t c_idx,
                                       int32_t bit_depth,
                                       int32_t mode,
                                       int32_t strong_intra_smoothing_enabled_flag,
                                       const int32_t *left,
                                       const int32_t *top,
                                       int32_t *left_out,
                                       int32_t *top_out);

#endif
