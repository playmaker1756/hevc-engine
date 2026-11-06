#include "intra_predict_test.h"

#include "expect.h"
#include "version0_picture.h"

#include "hevc/intra/predict.h"
#include "hevc/intra/reference.h"

#include <stddef.h>

/* Reference gather, substitution, smoothing, and the three predictors. */
void hevc_test_intra_predict(void)
{
    HevcPictureGeometry g;
    int32_t plane[64 * 64];
    int32_t left[65];
    int32_t left_av[65];
    int32_t top[64];
    int32_t top_av[64];
    int32_t left_f[65];
    int32_t top_f[64];
    int32_t pred[64];
    int32_t i;
    int32_t x;
    int32_t y;
    static const int32_t edge_left_missing[17] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    static const int32_t edge_top_present[16] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    };
    static const int32_t tr_left[17] = {
        0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0
    };
    static const int32_t dc_row0[8] = {18, 22, 22, 22, 22, 22, 22, 22};
    static const int32_t mode26[16] = {
        10, 20, 30, 40,
        30, 20, 30, 40,
        30, 20, 30, 40,
        30, 20, 30, 40
    };

    hevc_test_fill_version0(&g, 64, 64);
    hevc_test_expect_status("intra", "geometry", hevc_picture_geometry_derive(&g), HEVC_OK);
    for (y = 0; y < 64; y++) {
        for (x = 0; x < 64; x++) {
            plane[y * 64 + x] = y * 100 + x;
        }
    }

    hevc_test_expect_status("intra", "gather bottom-left 8x8",
                            hevc_intra_reference_gather(&g, plane, 64, 0, 0, 8, 8, 1, 1, 0,
                                                        left, left_av, top, top_av),
                            HEVC_OK);
    hevc_test_expect_row("intra", "bottom-left left arm missing", left_av, edge_left_missing, 17);
    hevc_test_expect_row("intra", "bottom-left top arm present", top_av, edge_top_present, 16);
    hevc_test_expect_i32("intra", "top sample is the reconstructed neighbour", top[3], 7 * 100 + 3);

    hevc_test_expect_status("intra", "gather top-right 8x8",
                            hevc_intra_reference_gather(&g, plane, 64, 0, 8, 0, 8, 1, 1, 0,
                                                        left, left_av, top, top_av),
                            HEVC_OK);
    hevc_test_expect_row("intra", "top-right left arm", left_av, tr_left, 17);
    hevc_test_expect_i32("intra", "left sample beside the block", left[1], 0 * 100 + 7);

    /* Whole strip missing becomes the 8-bit midpoint. */
    for (i = 0; i < 9; i++) {
        left_av[i] = 0;
        left[i] = 0;
    }
    for (i = 0; i < 8; i++) {
        top_av[i] = 0;
        top[i] = 0;
    }
    hevc_test_expect_status("intra", "substitute all missing",
                            hevc_intra_reference_substitute(4, 8, left, left_av, top, top_av),
                            HEVC_OK);
    hevc_test_expect_i32("intra", "midpoint", left[0], 128);
    hevc_test_expect_i32("intra", "midpoint top", top[7], 128);

    /* A hole between 1 and 9 inherits 1, the sample farther from the corner. */
    for (i = 0; i < 9; i++) {
        left_av[i] = 1;
        left[i] = 4;
    }
    for (i = 0; i < 8; i++) {
        top_av[i] = 1;
        top[i] = 4;
    }
    left[8] = 1;
    left_av[7] = 0;
    left[6] = 9;
    hevc_test_expect_status("intra", "substitute a hole",
                            hevc_intra_reference_substitute(4, 8, left, left_av, top, top_av),
                            HEVC_OK);
    hevc_test_expect_i32("intra", "hole copies the far sample", left[7], 1);
    hevc_test_expect_i32("intra", "present sample stays", left[6], 9);

    /* Picture-edge left arm copies the first top sample upward through the column. */
    for (i = 0; i < 17; i++) {
        left_av[i] = 0;
        left[i] = 0;
    }
    for (i = 0; i < 16; i++) {
        top_av[i] = 1;
        top[i] = 20 + i;
    }
    hevc_test_expect_status("intra", "substitute missing left arm",
                            hevc_intra_reference_substitute(8, 8, left, left_av, top, top_av),
                            HEVC_OK);
    hevc_test_expect_i32("intra", "left arm takes p[0][-1]", left[0], 20);
    hevc_test_expect_i32("intra", "far left takes p[0][-1]", left[16], 20);

    for (i = 0; i < 17; i++) {
        left[i] = 0;
    }
    for (i = 0; i < 16; i++) {
        top[i] = 0;
    }
    left[1] = 0;
    top[0] = 8;
    hevc_test_expect_status("intra", "smooth planar 8",
                            hevc_intra_reference_smooth(8, 0, 8, 0, 0, left, top, left_f, top_f),
                            HEVC_OK);
    hevc_test_expect_i32("intra", "3-tap corner", left_f[0], 2);
    hevc_test_expect_status("intra", "vertical is not smoothed at 8",
                            hevc_intra_reference_smooth(8, 0, 8, 26, 0, left, top, left_f, top_f),
                            HEVC_OK);
    hevc_test_expect_i32("intra", "unsmoothed corner", left_f[0], 0);

    for (i = 0; i < 65; i++) {
        left[i] = 0;
    }
    for (i = 0; i < 64; i++) {
        top[i] = 0;
    }
    left[0] = 0;
    left[32] = 32;
    left[64] = 64;
    top[31] = 32;
    top[63] = 64;
    hevc_test_expect_status("intra", "strong smooth",
                            hevc_intra_reference_smooth(32, 0, 8, 0, 1, left, top, left_f, top_f),
                            HEVC_OK);
    hevc_test_expect_i32("intra", "strong ramp at y = 0", left_f[1], 1);
    hevc_test_expect_i32("intra", "strong end stays", left_f[64], 64);

    for (i = 0; i < 9; i++) {
        left[i] = 0;
    }
    for (i = 0; i < 8; i++) {
        top[i] = 8;
    }
    left[1] = 10;
    left[2] = 20;
    left[3] = 30;
    left[4] = 40;
    left[5] = 50;
    left[6] = 60;
    left[7] = 70;
    left[8] = 80;
    hevc_test_expect_status("intra", "dc 8x8",
                            hevc_intra_predict_dc(8, 0, 8, 0, left, top, pred),
                            HEVC_OK);
    hevc_test_expect_row("intra", "dc top row", pred, dc_row0, 8);
    hevc_test_expect_i32("intra", "dc interior", pred[2 * 8 + 2], 27);
    hevc_test_expect_i32("intra", "dc left edge", pred[1 * 8], 25);

    left[1] = 1;
    left[2] = 2;
    left[3] = 3;
    left[4] = 4;
    left[5] = 9;
    top[0] = 5;
    top[1] = 6;
    top[2] = 7;
    top[3] = 8;
    top[4] = 10;
    hevc_test_expect_status("intra", "planar 4x4",
                            hevc_intra_predict_planar(4, left, top, pred),
                            HEVC_OK);
    hevc_test_expect_i32("intra", "planar origin", pred[0], 5);

    for (i = 0; i < 9; i++) {
        left[i] = 50;
    }
    left[0] = 10;
    left[1] = 10;
    left[2] = 50;
    left[3] = 50;
    left[4] = 50;
    top[0] = 10;
    top[1] = 20;
    top[2] = 30;
    top[3] = 40;
    hevc_test_expect_status("intra", "mode 26",
                            hevc_intra_predict_angular(4, 0, 8, 26, 0, left, top, pred),
                            HEVC_OK);
    hevc_test_expect_row("intra", "mode 26 block", pred, mode26, 16);

    top[0] = 132;
    top[1] = 160;
    left[0] = 100;
    hevc_test_expect_status("intra", "mode 25",
                            hevc_intra_predict_angular(4, 0, 8, 25, 1, left, top, pred),
                            HEVC_OK);
    hevc_test_expect_i32("intra", "negative angle fraction", pred[0], 130);
}
