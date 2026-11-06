#include "version0_picture.h"

#include <string.h>

/* Clear the struct, then set only the inputs this version fixes.
 * memset is the clear. Assigning each input afterwards keeps those values
 * visible instead of hiding them in a constructor. */
void hevc_test_fill_version0(HevcPictureGeometry *g, int32_t width, int32_t height)
{
    memset(g, 0, sizeof(*g));
    g->pic_width_in_luma_samples = width;
    g->pic_height_in_luma_samples = height;
    g->sub_width_c = 2;
    g->sub_height_c = 2;
    g->bit_depth_y = 8;
    g->bit_depth_c = 8;
    g->min_cb_log2_size_y = 3;
    g->ctb_log2_size_y = 6;
    g->min_tb_log2_size_y = 2;
}
