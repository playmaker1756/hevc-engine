#include "derived_rest_test.h"

#include "expect.h"

#include "hevc/bits/annex_b.h"
#include "hevc/bits/exp_golomb.h"
#include "hevc/common/integer_spec.h"
#include "hevc/intra/mode.h"
#include "hevc/loop/deblock.h"
#include "hevc/loop/sao.h"
#include "hevc/residual/inverse.h"

#include <string.h>

static void fill_side(int32_t side[4][4], int32_t sample)
{
    int32_t line;
    int32_t i;

    for (line = 0; line < 4; line++) {
        for (i = 0; i < 4; i++) {
            side[line][i] = sample;
        }
    }
}

/* Residual, deblock, SAO, Exp-Golomb, Annex B, and intra mode lists. */
void hevc_test_derived_rest(void)
{
    int32_t levels[16];
    int32_t residual[16];
    int32_t coeff;
    int32_t p[4][4];
    int32_t q[4][4];
    int32_t cp0[4];
    int32_t cp1[4];
    int32_t cq0[4];
    int32_t cq1[4];
    int32_t list[3];
    int32_t mode;
    int32_t k;
    HevcBitCursor cursor;
    int32_t value;
    int32_t pos;
    int32_t payload_at;
    int32_t payload_size;
    static const uint8_t ue_bits[1] = {0x20};
    static const uint8_t se_bits[1] = {0x60};
    static const uint8_t annex[12] = {
        0x00, 0x00, 0x00, 0x01, 0x65, 0xff, 0x00, 0x00, 0x00, 0x01, 0x41, 0x00
    };
    static const int32_t residual_expected[16] = {
        0, 0, 0, 0,
        0, 1, 1, 0,
        0, 1, 1, 1,
        0, 0, 1, 0
    };

    hevc_test_expect_i32("shift", "floor divide -2 by 32", hevc_floor_div_pow2(-2, 5), -1);
    hevc_test_expect_i32("shift", "floor divide -33 by 32", hevc_floor_div_pow2(-33, 5), -2);

    hevc_test_expect_status("residual", "scale level 8 at QP 4",
                            hevc_inv_scale_level(8, 4, 8, 2, 16, &coeff), HEVC_OK);
    hevc_test_expect_i32("residual", "scaled DC coefficient", coeff, 256);

    memset(levels, 0, sizeof(levels));
    levels[0] = 8;
    hevc_test_expect_status("residual", "4x4 sine residual",
                            hevc_inv_residual_4x4(levels, 4, 8, 1, 16, residual), HEVC_OK);
    hevc_test_expect_row("residual", "level 8 block", residual, residual_expected, 16);

    fill_side(p, 80);
    fill_side(q, 88);
    hevc_test_expect_status("deblock", "flat step",
                            hevc_deblock_luma_segment(p, q, 26, 26, 2, 0, 0, 8), HEVC_OK);
    hevc_test_expect_i32("deblock", "p0", p[0][0], 82);
    hevc_test_expect_i32("deblock", "p1", p[0][1], 81);
    hevc_test_expect_i32("deblock", "p2 stays", p[0][2], 80);
    hevc_test_expect_i32("deblock", "q0", q[0][0], 86);
    hevc_test_expect_i32("deblock", "q1", q[0][1], 87);

    fill_side(p, 80);
    fill_side(q, 82);
    hevc_test_expect_status("deblock", "strong step of 2",
                            hevc_deblock_luma_segment(p, q, 26, 26, 2, 0, 0, 8), HEVC_OK);
    hevc_test_expect_i32("deblock", "strong p0", p[0][0], 81);
    hevc_test_expect_i32("deblock", "strong q0", q[0][0], 81);
    hevc_test_expect_i32("deblock", "strong p2 stays", p[0][2], 80);

    fill_side(p, 100);
    fill_side(q, 100);
    for (k = 0; k < 4; k++) {
        q[k][0] = 108;
        q[k][1] = 120;
        q[k][2] = 132;
        q[k][3] = 132;
    }
    hevc_test_expect_status("deblock", "uneven weak step",
                            hevc_deblock_luma_segment(p, q, 26, 26, 2, 0, 0, 8), HEVC_OK);
    hevc_test_expect_i32("deblock", "uneven p0", p[0][0], 101);
    hevc_test_expect_i32("deblock", "uneven q0", q[0][0], 107);
    hevc_test_expect_i32("deblock", "uneven q1", q[0][1], 119);

    for (k = 0; k < 4; k++) {
        cp0[k] = 40;
        cp1[k] = 40;
        cq0[k] = 56;
        cq1[k] = 56;
    }
    hevc_test_expect_status("deblock", "chroma",
                            hevc_deblock_chroma_segment(cp0, cp1, cq0, cq1, 26, 26, 0, 0, 8),
                            HEVC_OK);
    hevc_test_expect_i32("deblock", "chroma p0", cp0[0], 42);
    hevc_test_expect_i32("deblock", "chroma q0", cq0[0], 54);
    hevc_test_expect_i32("deblock", "chroma p1 stays", cp1[0], 40);

    hevc_test_expect_i32("sao", "local minimum", hevc_sao_edge_category(10, 20, 20), 1);
    hevc_test_expect_i32("sao", "slope", hevc_sao_edge_category(10, 8, 12), 0);
    hevc_test_expect_i32("sao", "edge sign of category 1", hevc_sao_edge_sign(0), 0);
    hevc_test_expect_i32("sao", "edge sign of category 4", hevc_sao_edge_sign(3), 1);
    hevc_test_expect_i32("sao", "apply magnitude 3", hevc_sao_apply(10, 3, 8), 13);
    hevc_test_expect_i32("sao", "band of 128 at class 16", hevc_sao_band_category(128, 8, 16), 1);
    hevc_test_expect_i32("sao", "band miss", hevc_sao_band_category(40, 8, 16), 0);

    cursor.bytes = ue_bits;
    cursor.size_bytes = 1;
    cursor.bit_pos = 0;
    hevc_test_expect_status("golomb", "ue", hevc_bit_read_ue(&cursor, &value), HEVC_OK);
    hevc_test_expect_i32("golomb", "00100 is 3", value, 3);

    cursor.bytes = se_bits;
    cursor.size_bytes = 1;
    cursor.bit_pos = 0;
    hevc_test_expect_status("golomb", "se", hevc_bit_read_se(&cursor, &value), HEVC_OK);
    hevc_test_expect_i32("golomb", "code 2 is -1", value, -1);

    pos = 0;
    hevc_test_expect_status("annex b", "first nal",
                            hevc_annexb_next_nal(annex, 12, &pos, &payload_at, &payload_size),
                            HEVC_OK);
    hevc_test_expect_i32("annex b", "first payload size", payload_size, 2);
    hevc_test_expect_i32("annex b", "first payload byte", annex[payload_at], 0x65);
    hevc_test_expect_status("annex b", "second nal",
                            hevc_annexb_next_nal(annex, 12, &pos, &payload_at, &payload_size),
                            HEVC_OK);
    hevc_test_expect_i32("annex b", "second payload byte", annex[payload_at], 0x41);
    hevc_test_expect_i32("annex b", "second payload size", payload_size, 2);

    hevc_test_expect_status("mode", "both DC", hevc_intra_mpm_list(1, 1, list), HEVC_OK);
    hevc_test_expect_i32("mode", "mpm 0", list[0], 0);
    hevc_test_expect_i32("mode", "mpm 1", list[1], 1);
    hevc_test_expect_i32("mode", "mpm 2", list[2], 26);

    hevc_test_expect_status("mode", "both angular 10", hevc_intra_mpm_list(10, 10, list), HEVC_OK);
    hevc_test_expect_i32("mode", "neighbour below", list[1], 9);
    hevc_test_expect_i32("mode", "neighbour above", list[2], 11);

    hevc_test_expect_status("mode", "10 and 18", hevc_intra_mpm_list(10, 18, list), HEVC_OK);
    hevc_test_expect_i32("mode", "third is planar", list[2], 0);
    hevc_test_expect_status("mode", "remainder",
                            hevc_intra_mode_from_remainder(list, 9, &mode), HEVC_OK);
    hevc_test_expect_i32("mode", "remainder 9", mode, 11);

    hevc_test_expect_status("mode", "chroma planar luma",
                            hevc_chroma_intra_mode(0, 0, &mode), HEVC_OK);
    hevc_test_expect_i32("mode", "luma 0 syntax 0", mode, 34);
    hevc_test_expect_status("mode", "chroma vertical luma",
                            hevc_chroma_intra_mode(26, 0, &mode), HEVC_OK);
    hevc_test_expect_i32("mode", "luma 26 syntax 0", mode, 0);
    hevc_test_expect_status("mode", "chroma derived",
                            hevc_chroma_intra_mode(18, 4, &mode), HEVC_OK);
    hevc_test_expect_i32("mode", "syntax 4 copies luma", mode, 18);
}
