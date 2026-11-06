#include "picture_geometry_test.h"

#include "expect.h"
#include "version0_picture.h"

#include "hevc/picture/geometry.h"

#include <stddef.h>

/* Known pictures: a partial last tree row, a crop of 8 luma rows, and inputs
 * the deriver must refuse. */
void hevc_test_picture_geometry(void)
{
    HevcPictureGeometry g;
    int32_t output_width;
    int32_t output_height;

    hevc_test_fill_version0(&g, 1920, 1080);
    hevc_test_expect_status("picture geometry", "1920x1080 derives",
                            hevc_picture_geometry_derive(&g), HEVC_OK);
    hevc_test_expect_i32("picture geometry", "coding tree size", g.ctb_size_y, 64);
    hevc_test_expect_i32("picture geometry", "minimum coding block", g.min_cb_size_y, 8);
    hevc_test_expect_i32("picture geometry", "minimum transform block", g.min_tb_size_y, 4);
    hevc_test_expect_i32("picture geometry", "1920 luma samples cover 30 trees", g.pic_width_in_ctbs_y, 30);
    hevc_test_expect_i32("picture geometry", "1080 luma samples cover 17 trees", g.pic_height_in_ctbs_y, 17);
    hevc_test_expect_i32("picture geometry", "chroma width", g.pic_width_in_samples_c, 960);
    hevc_test_expect_i32("picture geometry", "chroma height", g.pic_height_in_samples_c, 540);
    hevc_test_expect_i32("picture geometry", "crop x0", g.crop_x0, 0);
    hevc_test_expect_i32("picture geometry", "crop y0", g.crop_y0, 0);
    hevc_test_expect_i32("picture geometry", "crop x1", g.crop_x1, 1919);
    hevc_test_expect_i32("picture geometry", "crop y1", g.crop_y1, 1079);
    output_width = g.crop_x1 - g.crop_x0 + 1;
    output_height = g.crop_y1 - g.crop_y0 + 1;
    hevc_test_expect_i32("picture geometry", "output width", output_width, 1920);
    hevc_test_expect_i32("picture geometry", "output height", output_height, 1080);

    hevc_test_fill_version0(&g, 1920, 1088);
    g.conf_win_bottom_offset = 4;
    hevc_test_expect_status("picture geometry", "padded 1088 derives",
                            hevc_picture_geometry_derive(&g), HEVC_OK);
    hevc_test_expect_i32("picture geometry", "1088 luma samples cover 17 trees", g.pic_height_in_ctbs_y, 17);
    hevc_test_expect_i32("picture geometry", "bottom offset 4 ends at luma row 1079", g.crop_y1, 1079);
    output_height = g.crop_y1 - g.crop_y0 + 1;
    hevc_test_expect_i32("picture geometry", "output height after the bottom crop", output_height, 1080);

    hevc_test_fill_version0(&g, 4, 8);
    hevc_test_expect_status("picture geometry", "width 4 is below the minimum coding block",
                            hevc_picture_geometry_derive(&g), HEVC_ERR_RANGE);

    hevc_test_fill_version0(&g, 64, 64);
    g.conf_win_left_offset = 16;
    g.conf_win_right_offset = 16;
    hevc_test_expect_status("picture geometry", "a crop of the whole picture is rejected",
                            hevc_picture_geometry_derive(&g), HEVC_ERR_RANGE);

    hevc_test_expect_status("picture geometry", "null geometry",
                            hevc_picture_geometry_derive(NULL), HEVC_ERR_ARG);

    hevc_test_fill_version0(&g, 64, 64);
    g.conf_win_left_offset = 2147483647;
    g.conf_win_right_offset = 1;
    hevc_test_expect_status("picture geometry", "a crop offset that overflows a signed sum is rejected",
                            hevc_picture_geometry_derive(&g), HEVC_ERR_RANGE);

    hevc_test_fill_version0(&g, 64, 64);
    g.min_tb_log2_size_y = 3;
    hevc_test_expect_status("picture geometry", "minimum transform block equal to the coding block is rejected",
                            hevc_picture_geometry_derive(&g), HEVC_ERR_RANGE);
}
