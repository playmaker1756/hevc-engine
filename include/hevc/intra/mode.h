#ifndef HEVC_INTRA_MODE_H
#define HEVC_INTRA_MODE_H

#include "hevc/status.h"

#include <stdint.h>

/* Clause 8.4.2. Build the three most-probable luma modes.
 * cand_a is the left neighbour and cand_b is the above neighbour.
 * Pass 1 for a missing neighbour, a non-intra neighbour, or an above
 * neighbour that crosses into the previous coding-tree row. That last case
 * is DC even when the sample itself is available for prediction.
 */
HevcStatus hevc_intra_mpm_list(int32_t cand_a, int32_t cand_b, int32_t list[3]);

/* The mode coded by rem_intra_luma_pred_mode. The list is sorted, then each
 * candidate at or below the running value bumps that value by one.
 */
HevcStatus hevc_intra_mode_from_remainder(const int32_t list[3], int32_t remainder, int32_t *mode);

/* Clause 8.4.3, Table 8-2. 4:2:0 copies modeIdx. The 4:2:2 remap is not
 * applied. chroma_syntax is intra_chroma_pred_mode, 0..4.
 */
HevcStatus hevc_chroma_intra_mode(int32_t luma_mode, int32_t chroma_syntax, int32_t *mode);

#endif
