#include "neighbor_availability_test.h"

#include "expect.h"
#include "version0_picture.h"

#include "hevc/picture/neighbor_availability.h"

/* Addresses and neighbour decisions for the 8x8 maps already derived.
 * 1 means the sample may be read. 0 means it may not. */
void hevc_test_neighbor_availability(void)
{
    HevcPictureGeometry g;
    HevcPictureGeometry wide;
    int32_t actual[17];
    int32_t x;
    int32_t y;
    int32_t i;
    int32_t sample_offset;
    static const int32_t origin_grid[4][4] = {
        {0, 1, 4, 5},
        {2, 3, 6, 7},
        {8, 9, 12, 13},
        {10, 11, 14, 15},
    };
    static const int32_t outside_left[17] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    static const int32_t bottom_left_top[16] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    };
    static const int32_t top_right_left[17] = {
        0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    static const int32_t interior_top[16] = {
        1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    hevc_test_fill_version0(&g, 64, 64);
    hevc_test_expect_status("neighbor", "64x64 derives",
                            hevc_picture_geometry_derive(&g), HEVC_OK);

    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            hevc_test_expect_i32("neighbor", "address of a 4x4 inside the first tree",
                                 hevc_min_tb_addr_zs(&g, x, y), origin_grid[y][x]);
        }
    }
    hevc_test_expect_i32("neighbor", "top-left 8x8 at luma (16, 16)", hevc_min_tb_addr_zs(&g, 4, 4), 48);
    hevc_test_expect_i32("neighbor", "top-right 8x8 at luma (24, 16)", hevc_min_tb_addr_zs(&g, 6, 4), 52);
    hevc_test_expect_i32("neighbor", "bottom-left 8x8 at luma (16, 24)", hevc_min_tb_addr_zs(&g, 4, 6), 56);
    hevc_test_expect_i32("neighbor", "bottom-right 8x8 at luma (24, 24)", hevc_min_tb_addr_zs(&g, 6, 6), 60);

    hevc_test_fill_version0(&wide, 128, 64);
    hevc_test_expect_status("neighbor", "128x64 derives",
                            hevc_picture_geometry_derive(&wide), HEVC_OK);
    hevc_test_expect_i32("neighbor", "first 4x4 of the second tree", hevc_min_tb_addr_zs(&wide, 16, 0), 256);

    hevc_test_expect_i32("neighbor", "same 4x4",
                         hevc_neighbor_available(&g, 0, 0, 3, 3, 1, 1), 1);
    hevc_test_expect_i32("neighbor", "the 4x4 to the right is later",
                         hevc_neighbor_available(&g, 0, 0, 4, 0, 1, 1), 0);
    hevc_test_expect_i32("neighbor", "the 4x4 to the left is earlier",
                         hevc_neighbor_available(&g, 4, 0, 0, 0, 1, 1), 1);
    hevc_test_expect_i32("neighbor", "a different slice",
                         hevc_neighbor_available(&g, 4, 0, 0, 0, 0, 1), 0);
    hevc_test_expect_i32("neighbor", "current sample outside the picture",
                         hevc_neighbor_available(&g, -1, 0, 0, 0, 1, 1), -1);

    /* Left arm of the 8x8 at (0, 8), from the corner downward. x is -1. */
    for (i = 0; i < 17; i++) {
        sample_offset = i - 1;
        actual[i] = hevc_neighbor_available(&g, 0, 8, -1, 8 + sample_offset, 1, 1);
    }
    hevc_test_expect_row("neighbor", "picture-edge bottom-left left arm", actual, outside_left, 17);

    /* Top arm of that block, including the samples that sit in the top-right 8x8. */
    for (i = 0; i < 16; i++) {
        actual[i] = hevc_neighbor_available(&g, 0, 8, i, 7, 1, 1);
    }
    hevc_test_expect_row("neighbor", "picture-edge bottom-left top arm", actual, bottom_left_top, 16);

    /* Left arm of the 8x8 at (8, 0). The lower half is still in the future. */
    for (i = 0; i < 17; i++) {
        sample_offset = i - 1;
        actual[i] = hevc_neighbor_available(&g, 8, 0, 7, sample_offset, 1, 1);
    }
    hevc_test_expect_row("neighbor", "picture-edge top-right left arm", actual, top_right_left, 17);

    /* Top arm of the 8x8 at (24, 16). The extension crosses x = 32. */
    for (i = 0; i < 16; i++) {
        actual[i] = hevc_neighbor_available(&g, 24, 16, 24 + i, 15, 1, 1);
    }
    hevc_test_expect_row("neighbor", "interior top-right top arm", actual, interior_top, 16);
}
