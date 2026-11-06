#ifndef HEVC_LOOP_SAO_H
#define HEVC_LOOP_SAO_H

#include "hevc/status.h"

#include <stdint.h>

/* Sample adaptive offset for one sample. Clause 8.7.3.2 and the sign
 * inference in 7.4.9.3. The CTB loop, merge flags, and slice boundaries
 * stay with the caller. These functions are the per-sample arithmetic.
 */

/* Edge category 0..4. Neighbours are the two samples named by the class.
 * A local minimum is category 1. A local maximum is category 4. A slope
 * is category 0 and adds nothing.
 */
int32_t hevc_sao_edge_category(int32_t center, int32_t neighbour0, int32_t neighbour1);

/* Band category 0..4. left_class is sao_band_position. Category 0 means
 * the sample is outside the four marked bands.
 */
int32_t hevc_sao_band_category(int32_t sample, int32_t bit_depth, int32_t left_class);

/* Edge-offset sign for category slot i in 0..3, before it is stored as
 * SaoOffsetVal[i + 1]. Categories 1 and 2 are positive (sign 0). Categories
 * 3 and 4 are negative (sign 1). Band offset codes its own sign.
 */
int32_t hevc_sao_edge_sign(int32_t category_slot);

/* Clip sample + offset into the bit depth. offset already carries its sign. */
int32_t hevc_sao_apply(int32_t sample, int32_t offset, int32_t bit_depth);

#endif
