#ifndef HEVC_PICTURE_CODED_STRUCTURE_H
#define HEVC_PICTURE_CODED_STRUCTURE_H

#include "hevc/status.h"

#include <stdint.h>

/* Clause 7.3.8.4 and 7.4.9.4. Decide split_cu_flag.
 * *present is 1 when the flag is in the bitstream. *split is then coded_flag.
 * *present is 0 when the block is not fully inside the picture or it is
 * already the minimum coding block. *split is then 1 when the block is still
 * larger than the minimum, and 0 when it is the minimum.
 */
HevcStatus hevc_split_cu(int32_t x0,
                         int32_t y0,
                         int32_t log2_cb_size,
                         int32_t min_cb_log2_size_y,
                         int32_t pic_width,
                         int32_t pic_height,
                         int32_t coded_flag,
                         int32_t *present,
                         int32_t *split);

/* Clause 6.5.1. Fill raster-to-tile-scan and tile-scan-to-raster maps.
 * Column widths and row heights are in coding-tree blocks.
 * map_count must equal the sum of the widths times the sum of the heights.
 */
HevcStatus hevc_ctb_tile_scan(const int32_t *col_width,
                              int32_t num_cols,
                              const int32_t *row_height,
                              int32_t num_rows,
                              int32_t *rs_to_ts,
                              int32_t *ts_to_rs,
                              int32_t map_count);

/* Equation 9-3. The wavefront source is the second coding tree of the row
 * above: one tree to the right and one tree up. The caller tests availability
 * with clause 6.4.1. y_nb is negative on the first row.
 */
void hevc_wpp_source(int32_t x0, int32_t y0, int32_t ctb_size, int32_t *x_nb, int32_t *y_nb);

/* Equation 8-12. Place one PCM sample into the reconstructed picture.
 * The shift is BitDepth minus PcmBitDepth and is non-negative.
 */
HevcStatus hevc_pcm_sample(int32_t pcm_sample,
                           int32_t bit_depth,
                           int32_t pcm_bit_depth,
                           int32_t *reconstructed);

/* Equation 8-309's weight. The weight is 16 when scaling lists are off, and
 * also when transform skip is used on a block larger than 4. Otherwise it is
 * the list entry the caller already derived.
 */
int32_t hevc_scaling_weight(int32_t scaling_list_enabled,
                            int32_t transform_skip,
                            int32_t n_tb_s,
                            int32_t list_weight);

#endif
