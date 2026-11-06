#include "hevc/picture/geometry.h"

#include <stddef.h>
#include <stdint.h>

/* Ceiling of n/d for positive n and d.
 * (n + d - 1) / d is that ceiling in integer division. A floating divide
 * would round in a way this grid is not allowed to use.
 */
static int32_t ceil_div_positive(int32_t n, int32_t d)
{
    return (n + d - 1) / d;
}

/* Derive the coding-tree grid and the output window.
 * Inputs that cannot describe a Main-family picture are rejected before any
 * derived field is written, so a failed call does not leave a half grid.
 */
HevcStatus hevc_picture_geometry_derive(HevcPictureGeometry *g)
{
    int32_t min_cb;
    int32_t ctb;
    int32_t min_tb;
    int64_t crop_w64;
    int64_t crop_h64;

    if (g == NULL) {
        return HEVC_ERR_ARG;
    }

    /* A picture has a positive decoded width and height. */
    if (g->pic_width_in_luma_samples <= 0 || g->pic_height_in_luma_samples <= 0) {
        return HEVC_ERR_RANGE;
    }
    /* Table 6-1 only defines chroma steps of 1 and 2. */
    if ((g->sub_width_c != 1 && g->sub_width_c != 2) ||
        (g->sub_height_c != 1 && g->sub_height_c != 2)) {
        return HEVC_ERR_RANGE;
    }
    /* Clause 6.2: each sample uses 8 to 16 bits. */
    if (g->bit_depth_y < 8 || g->bit_depth_y > 16 ||
        g->bit_depth_c < 8 || g->bit_depth_c > 16) {
        return HEVC_ERR_RANGE;
    }
    /* Minimum coding block is 8..64. The coding tree is at least that
     * large and at most 64. */
    if (g->min_cb_log2_size_y < 3 || g->min_cb_log2_size_y > 6 ||
        g->ctb_log2_size_y < g->min_cb_log2_size_y || g->ctb_log2_size_y > 6) {
        return HEVC_ERR_RANGE;
    }
    /* Minimum transform block is 4..32 and must be smaller than the
     * minimum coding block, or the transform tree has no legal leaf. */
    if (g->min_tb_log2_size_y < 2 || g->min_tb_log2_size_y > 5 ||
        g->min_tb_log2_size_y >= g->min_cb_log2_size_y) {
        return HEVC_ERR_RANGE;
    }
    /* A negative crop offset is not a window. */
    if (g->conf_win_left_offset < 0 || g->conf_win_right_offset < 0 ||
        g->conf_win_top_offset < 0 || g->conf_win_bottom_offset < 0) {
        return HEVC_ERR_RANGE;
    }

    /* These shifts are non-negative log2 sizes already range-checked,
     * so the result is the block size in samples. */
    min_cb = 1 << g->min_cb_log2_size_y;
    ctb = 1 << g->ctb_log2_size_y;
    min_tb = 1 << g->min_tb_log2_size_y;

    /* Width and height must cover whole minimum coding blocks, and whole
     * chroma steps, or the chroma arrays would not be an integer size. */
    if ((g->pic_width_in_luma_samples % min_cb) != 0 ||
        (g->pic_height_in_luma_samples % min_cb) != 0 ||
        (g->pic_width_in_luma_samples % g->sub_width_c) != 0 ||
        (g->pic_height_in_luma_samples % g->sub_height_c) != 0) {
        return HEVC_ERR_RANGE;
    }

    /* Offsets are in chroma samples. The product is formed in 64 bits so a
     * huge offset cannot wrap and look smaller than the picture. */
    crop_w64 = (int64_t)g->sub_width_c *
               ((int64_t)g->conf_win_left_offset + (int64_t)g->conf_win_right_offset);
    crop_h64 = (int64_t)g->sub_height_c *
               ((int64_t)g->conf_win_top_offset + (int64_t)g->conf_win_bottom_offset);
    if (crop_w64 >= g->pic_width_in_luma_samples || crop_h64 >= g->pic_height_in_luma_samples) {
        return HEVC_ERR_RANGE;
    }

    g->min_cb_size_y = min_cb;
    g->ctb_size_y = ctb;
    g->min_tb_size_y = min_tb;
    /* Cover a width that is not a multiple of the tree. Truncating division
     * would drop the partial tree on the right or the bottom. */
    g->pic_width_in_ctbs_y = ceil_div_positive(g->pic_width_in_luma_samples, ctb);
    g->pic_height_in_ctbs_y = ceil_div_positive(g->pic_height_in_luma_samples, ctb);
    g->pic_width_in_samples_c = g->pic_width_in_luma_samples / g->sub_width_c;
    g->pic_height_in_samples_c = g->pic_height_in_luma_samples / g->sub_height_c;
    g->crop_x0 = g->sub_width_c * g->conf_win_left_offset;
    g->crop_y0 = g->sub_height_c * g->conf_win_top_offset;
    /* The end index is inclusive, so one is subtracted after the removed
     * samples. A zero offset ends at width - 1. */
    g->crop_x1 = g->pic_width_in_luma_samples - (g->sub_width_c * g->conf_win_right_offset + 1);
    g->crop_y1 = g->pic_height_in_luma_samples - (g->sub_height_c * g->conf_win_bottom_offset + 1);
    return HEVC_OK;
}
