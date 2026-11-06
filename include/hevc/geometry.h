#ifndef HEVC_GEOMETRY_H
#define HEVC_GEOMETRY_H

#include "hevc/check.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Picture grid from clauses 6.2 and 7.4.3.
 * The caller fills the input fields. hevc_geometry_derive writes the derived
 * fields. Internal decoding uses the uncropped picture. crop_x0..crop_y1 is
 * the conformance window in inclusive luma coordinates, applied only on output.
 */
typedef struct HevcGeometry {
    int pic_width_in_luma_samples;
    int pic_height_in_luma_samples;
    int sub_width_c;
    int sub_height_c;
    int bit_depth_y;
    int bit_depth_c;
    int min_cb_log2_size_y;
    int ctb_log2_size_y;
    int conf_win_left_offset;
    int conf_win_right_offset;
    int conf_win_top_offset;
    int conf_win_bottom_offset;

    int min_cb_size_y;
    int ctb_size_y;
    int pic_width_in_ctbs_y;
    int pic_height_in_ctbs_y;
    int pic_width_in_samples_c;
    int pic_height_in_samples_c;
    int crop_x0;
    int crop_y0;
    int crop_x1;
    int crop_y1;
} HevcGeometry;

HevcStatus hevc_geometry_derive(HevcGeometry *g);

#ifdef __cplusplus
}
#endif

#endif
