#include "hevc/picture/coded_structure.h"

#include <stddef.h>

/* Infer or accept split_cu_flag for one coding-quadtree node. */
HevcStatus hevc_split_cu(int32_t x0,
                         int32_t y0,
                         int32_t log2_cb_size,
                         int32_t min_cb_log2_size_y,
                         int32_t pic_width,
                         int32_t pic_height,
                         int32_t coded_flag,
                         int32_t *present,
                         int32_t *split)
{
    int32_t size;
    int32_t inside;

    if (present == NULL || split == NULL) {
        return HEVC_ERR_ARG;
    }
    if (log2_cb_size < 3 || log2_cb_size > 6 || min_cb_log2_size_y < 3) {
        return HEVC_ERR_RANGE;
    }
    size = 1 << log2_cb_size;
    inside = 0;
    if (x0 >= 0 && y0 >= 0 && x0 + size <= pic_width && y0 + size <= pic_height &&
        log2_cb_size > min_cb_log2_size_y) {
        inside = 1;
    }
    if (inside == 1) {
        *present = 1;
        *split = coded_flag;
        return HEVC_OK;
    }
    *present = 0;
    if (log2_cb_size > min_cb_log2_size_y) {
        *split = 1;
    } else {
        *split = 0;
    }
    return HEVC_OK;
}

/* Build the tile-scan address maps. One tile makes the map the identity. */
HevcStatus hevc_ctb_tile_scan(const int32_t *col_width,
                              int32_t num_cols,
                              const int32_t *row_height,
                              int32_t num_rows,
                              int32_t *rs_to_ts,
                              int32_t *ts_to_rs,
                              int32_t map_count)
{
    int32_t pic_w;
    int32_t pic_h;
    int32_t col;
    int32_t row;
    int32_t ts;
    int32_t x0;
    int32_t y0;

    if (col_width == NULL || row_height == NULL || rs_to_ts == NULL || ts_to_rs == NULL) {
        return HEVC_ERR_ARG;
    }
    if (num_cols <= 0 || num_rows <= 0) {
        return HEVC_ERR_RANGE;
    }

    pic_w = 0;
    for (col = 0; col < num_cols; col++) {
        if (col_width[col] <= 0) {
            return HEVC_ERR_RANGE;
        }
        pic_w = pic_w + col_width[col];
    }
    pic_h = 0;
    for (row = 0; row < num_rows; row++) {
        if (row_height[row] <= 0) {
            return HEVC_ERR_RANGE;
        }
        pic_h = pic_h + row_height[row];
    }
    if (pic_w * pic_h != map_count) {
        return HEVC_ERR_RANGE;
    }

    /* Tiles in raster order. Inside a tile, coding trees in raster order. */
    ts = 0;
    y0 = 0;
    for (row = 0; row < num_rows; row++) {
        x0 = 0;
        for (col = 0; col < num_cols; col++) {
            int32_t y;
            int32_t x;

            for (y = 0; y < row_height[row]; y++) {
                for (x = 0; x < col_width[col]; x++) {
                    int32_t rs;

                    rs = (y0 + y) * pic_w + (x0 + x);
                    rs_to_ts[rs] = ts;
                    ts_to_rs[ts] = rs;
                    ts = ts + 1;
                }
            }
            x0 = x0 + col_width[col];
        }
        y0 = y0 + row_height[row];
    }
    return HEVC_OK;
}

/* Name the coding tree whose CABAC state starts the next wavefront row. */
void hevc_wpp_source(int32_t x0, int32_t y0, int32_t ctb_size, int32_t *x_nb, int32_t *y_nb)
{
    *x_nb = x0 + ctb_size;
    *y_nb = y0 - ctb_size;
}

/* Shift a PCM sample up to the picture bit depth. */
HevcStatus hevc_pcm_sample(int32_t pcm_sample,
                           int32_t bit_depth,
                           int32_t pcm_bit_depth,
                           int32_t *reconstructed)
{
    int32_t shift;

    if (reconstructed == NULL) {
        return HEVC_ERR_ARG;
    }
    if (pcm_bit_depth < 1 || pcm_bit_depth > bit_depth || bit_depth > 16) {
        return HEVC_ERR_RANGE;
    }
    shift = bit_depth - pcm_bit_depth;
    *reconstructed = pcm_sample << shift;
    return HEVC_OK;
}

/* Choose the inverse-scaling weight for one coefficient. */
int32_t hevc_scaling_weight(int32_t scaling_list_enabled,
                            int32_t transform_skip,
                            int32_t n_tb_s,
                            int32_t list_weight)
{
    if (scaling_list_enabled == 0) {
        return 16;
    }
    if (transform_skip == 1 && n_tb_s > 4) {
        return 16;
    }
    return list_weight;
}
