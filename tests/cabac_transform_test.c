#include "cabac_transform_test.h"

#include "expect.h"

#include "hevc/bits/cabac.h"
#include "hevc/bits/rbsp.h"
#include "hevc/picture/coded_structure.h"
#include "hevc/residual/inverse.h"

#include <stdint.h>
#include <string.h>

/* One context bin from a zero bitstream, then the cosine, the skip, and the
 * picture rules that were re-read with the range table. */
void hevc_test_cabac_transform(void)
{
    uint8_t zeros[2];
    HevcBitCursor cursor;
    HevcCabacEngine engine;
    HevcCabacContext ctx;
    int32_t bin;
    int32_t finished;
    int32_t present;
    int32_t split;
    int32_t col_width[2];
    int32_t row_height[1];
    int32_t rs_to_ts[8];
    int32_t ts_to_rs[8];
    int32_t x_nb;
    int32_t y_nb;
    int32_t sample;
    uint8_t nal[6];
    uint8_t rbsp[6];
    int32_t rbsp_size;
    int32_t levels[16];
    int32_t residual[16];
    int32_t cosine[16];
    int32_t i;

    zeros[0] = 0;
    zeros[1] = 0;
    cursor.bytes = zeros;
    cursor.size_bytes = 2;
    cursor.bit_pos = 0;
    hevc_test_expect_status("cabac", "engine init", hevc_cabac_engine_init(&engine, &cursor), HEVC_OK);
    /* 510 >> 6 is 7, and 7 & 3 is 3. State 0, column 3 is the LPS range 240.
     * The MPS interval is 270. Offset 0 stays in it, so the bin is valMps. */
    hevc_test_expect_i32("cabac", "range after init", engine.range, 510);
    ctx.p_state = 0;
    ctx.val_mps = 0;
    hevc_test_expect_status("cabac", "decision", hevc_cabac_decision(&engine, &ctx, &bin), HEVC_OK);
    hevc_test_expect_i32("cabac", "first bin", bin, 0);
    hevc_test_expect_i32("cabac", "mps transition", ctx.p_state, 1);
    hevc_test_expect_i32("cabac", "mps range", engine.range, 270);

    hevc_test_expect_status("cabac", "bypass", hevc_cabac_bypass(&engine, &bin), HEVC_OK);
    hevc_test_expect_i32("cabac", "bypass bin", bin, 0);
    hevc_test_expect_status("cabac", "terminate", hevc_cabac_terminate(&engine, &bin, &finished), HEVC_OK);
    hevc_test_expect_i32("cabac", "terminate bin", bin, 0);
    hevc_test_expect_i32("cabac", "terminate continues", finished, 0);

    /* initValue 16: slope 1, offset 0, m = -40, n = -16. At QP 26 the
     * clipped state is 1, so valMps is 0 and pStateIdx is 62. */
    hevc_test_expect_status("cabac", "context init", hevc_cabac_context_init(16, 26, &ctx), HEVC_OK);
    hevc_test_expect_i32("cabac", "init valMps", ctx.val_mps, 0);
    hevc_test_expect_i32("cabac", "init state", ctx.p_state, 62);
    hevc_test_expect_i32("cabac", "I init type", hevc_cabac_init_type(2, 0), 0);

    nal[0] = 0;
    nal[1] = 1;
    nal[2] = 0;
    nal[3] = 0;
    nal[4] = 3;
    nal[5] = 1;
    hevc_test_expect_status("rbsp", "prevention", hevc_rbsp_from_nal(nal, 6, rbsp, 6, &rbsp_size), HEVC_OK);
    hevc_test_expect_i32("rbsp", "size", rbsp_size, 3);
    hevc_test_expect_i32("rbsp", "kept zero", rbsp[0], 0);
    hevc_test_expect_i32("rbsp", "second zero", rbsp[1], 0);
    hevc_test_expect_i32("rbsp", "payload", rbsp[2], 1);

    hevc_test_expect_status(
        "split", "coded",
        hevc_split_cu(0, 0, 6, 3, 64, 64, 0, &present, &split), HEVC_OK);
    hevc_test_expect_i32("split", "present", present, 1);
    hevc_test_expect_i32("split", "flag", split, 0);
    hevc_test_expect_status(
        "split", "past the picture",
        hevc_split_cu(0, 0, 6, 3, 32, 32, 0, &present, &split), HEVC_OK);
    hevc_test_expect_i32("split", "inferred present", present, 0);
    hevc_test_expect_i32("split", "inferred split", split, 1);

    col_width[0] = 2;
    col_width[1] = 2;
    row_height[0] = 2;
    hevc_test_expect_status(
        "tile", "scan",
        hevc_ctb_tile_scan(col_width, 2, row_height, 1, rs_to_ts, ts_to_rs, 8), HEVC_OK);
    hevc_test_expect_i32("tile", "second column starts later", rs_to_ts[2], 4);
    hevc_test_expect_i32("tile", "left tile continues down", rs_to_ts[4], 2);

    hevc_wpp_source(0, 64, 64, &x_nb, &y_nb);
    hevc_test_expect_i32("wpp", "source x", x_nb, 64);
    hevc_test_expect_i32("wpp", "source y", y_nb, 0);

    hevc_test_expect_status("pcm", "shift", hevc_pcm_sample(15, 8, 4, &sample), HEVC_OK);
    hevc_test_expect_i32("pcm", "reconstructed", sample, 240);

    hevc_test_expect_i32("scale", "lists off", hevc_scaling_weight(0, 0, 4, 40), 16);
    hevc_test_expect_i32("scale", "skip above 4", hevc_scaling_weight(1, 1, 8, 40), 16);
    hevc_test_expect_i32("scale", "list entry", hevc_scaling_weight(1, 0, 4, 40), 40);

    memset(levels, 0, sizeof(levels));
    levels[0] = 8;
    hevc_test_expect_status(
        "cosine", "4x4",
        hevc_inv_residual(levels, 4, 4, 8, 0, 0, 16, residual), HEVC_OK);
    cosine[0] = 2;
    cosine[1] = 3;
    cosine[2] = 2;
    cosine[3] = 1;
    cosine[4] = 3;
    cosine[5] = 3;
    cosine[6] = 3;
    cosine[7] = 1;
    cosine[8] = 2;
    cosine[9] = 3;
    cosine[10] = 2;
    cosine[11] = 1;
    cosine[12] = 1;
    cosine[13] = 1;
    cosine[14] = 1;
    cosine[15] = 1;
    hevc_test_expect_row("cosine", "dc impulse", residual, cosine, 16);

    hevc_test_expect_status(
        "skip", "4x4",
        hevc_inv_residual(levels, 4, 4, 8, 0, 1, 16, residual), HEVC_OK);
    for (i = 0; i < 16; i++) {
        cosine[i] = 0;
    }
    cosine[0] = 8;
    hevc_test_expect_row("skip", "scaled 256 becomes 8", residual, cosine, 16);
}
