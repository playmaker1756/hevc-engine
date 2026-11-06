#ifndef HEVC_PICTURE_GEOMETRY_H
#define HEVC_PICTURE_GEOMETRY_H

#include "hevc/status.h"

#include <stdint.h>

/* Decoded-picture grid. Clauses 6.2 and 7.4.3.
 * The caller writes the input fields. hevc_picture_geometry_derive writes
 * the derived fields. Decode uses the uncropped size. crop_x0..crop_y1 is
 * the conformance window in inclusive luma samples, used only at output.
 */
typedef struct HevcPictureGeometry {
    int32_t pic_width_in_luma_samples;
    int32_t pic_height_in_luma_samples;
    int32_t sub_width_c;
    int32_t sub_height_c;
    int32_t bit_depth_y;
    int32_t bit_depth_c;
    int32_t min_cb_log2_size_y;
    int32_t ctb_log2_size_y;
    int32_t min_tb_log2_size_y;
    int32_t conf_win_left_offset;
    int32_t conf_win_right_offset;
    int32_t conf_win_top_offset;
    int32_t conf_win_bottom_offset;

    int32_t min_cb_size_y;
    int32_t ctb_size_y;
    int32_t min_tb_size_y;
    int32_t pic_width_in_ctbs_y;
    int32_t pic_height_in_ctbs_y;
    int32_t pic_width_in_samples_c;
    int32_t pic_height_in_samples_c;
    int32_t crop_x0;
    int32_t crop_y0;
    int32_t crop_x1;
    int32_t crop_y1;
} HevcPictureGeometry;

/* Fill the derived fields from the inputs.
 * Returns HEVC_ERR_ARG when g is null.
 * Returns HEVC_ERR_RANGE when an input cannot describe a picture.
 */
HevcStatus hevc_picture_geometry_derive(HevcPictureGeometry *g);

#endif
