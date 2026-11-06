#include "hevc/geometry.h"

static int ceil_div_positive(int n, int d)
{
    return (n + d - 1) / d;
}

HevcStatus hevc_geometry_derive(HevcGeometry *g)
{
    int min_cb;
    int ctb;
    int crop_w;
    int crop_h;

    if (g == 0) {
        return HEVC_ERR_ARG;
    }
    if (g->pic_width_in_luma_samples <= 0 || g->pic_height_in_luma_samples <= 0) {
        return HEVC_ERR_RANGE;
    }
    if ((g->sub_width_c != 1 && g->sub_width_c != 2) ||
        (g->sub_height_c != 1 && g->sub_height_c != 2)) {
        return HEVC_ERR_RANGE;
    }
    if (g->bit_depth_y < 8 || g->bit_depth_y > 16 ||
        g->bit_depth_c < 8 || g->bit_depth_c > 16) {
        return HEVC_ERR_RANGE;
    }
    if (g->min_cb_log2_size_y < 3 || g->min_cb_log2_size_y > 6 ||
        g->ctb_log2_size_y < g->min_cb_log2_size_y || g->ctb_log2_size_y > 6) {
        return HEVC_ERR_RANGE;
    }
    if (g->conf_win_left_offset < 0 || g->conf_win_right_offset < 0 ||
        g->conf_win_top_offset < 0 || g->conf_win_bottom_offset < 0) {
        return HEVC_ERR_RANGE;
    }

    min_cb = 1 << g->min_cb_log2_size_y;
    ctb = 1 << g->ctb_log2_size_y;
    if ((g->pic_width_in_luma_samples % min_cb) != 0 ||
        (g->pic_height_in_luma_samples % min_cb) != 0 ||
        (g->pic_width_in_luma_samples % g->sub_width_c) != 0 ||
        (g->pic_height_in_luma_samples % g->sub_height_c) != 0) {
        return HEVC_ERR_RANGE;
    }

    crop_w = g->sub_width_c * (g->conf_win_left_offset + g->conf_win_right_offset);
    crop_h = g->sub_height_c * (g->conf_win_top_offset + g->conf_win_bottom_offset);
    if (crop_w >= g->pic_width_in_luma_samples || crop_h >= g->pic_height_in_luma_samples) {
        return HEVC_ERR_RANGE;
    }

    g->min_cb_size_y = min_cb;
    g->ctb_size_y = ctb;
    g->pic_width_in_ctbs_y = ceil_div_positive(g->pic_width_in_luma_samples, ctb);
    g->pic_height_in_ctbs_y = ceil_div_positive(g->pic_height_in_luma_samples, ctb);
    g->pic_width_in_samples_c = g->pic_width_in_luma_samples / g->sub_width_c;
    g->pic_height_in_samples_c = g->pic_height_in_luma_samples / g->sub_height_c;
    g->crop_x0 = g->sub_width_c * g->conf_win_left_offset;
    g->crop_y0 = g->sub_height_c * g->conf_win_top_offset;
    g->crop_x1 = g->pic_width_in_luma_samples - (g->sub_width_c * g->conf_win_right_offset + 1);
    g->crop_y1 = g->pic_height_in_luma_samples - (g->sub_height_c * g->conf_win_bottom_offset + 1);
    return HEVC_OK;
}
