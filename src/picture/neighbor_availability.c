#include "hevc/picture/neighbor_availability.h"

#include <stddef.h>

/* Address of one minimum transform block in quadtree order.
 * The value is recomputed from the formula instead of read from a
 * picture-sized MinTbAddrZs table. A 64-sample tree interleaves four bits.
 * A table would replace those shifts with a load and would hide the order.
 * If a profile later shows this function as hot, a table may replace it,
 * and the addresses must stay the same.
 */
int32_t hevc_min_tb_addr_zs(const HevcPictureGeometry *g, int32_t x_tb, int32_t y_tb)
{
    int32_t diff;
    int32_t grid_w;
    int32_t grid_h;
    int32_t ctb_x;
    int32_t ctb_y;
    int32_t ctb_addr_rs;
    int32_t ctb_addr_ts;
    int32_t addr;
    int32_t i;
    int32_t m;
    int32_t p;

    if (g == NULL || g->ctb_size_y <= 0 || g->min_tb_size_y <= 0) {
        return -1;
    }

    /* How many times the minimum transform block fits along one tree edge. */
    diff = g->ctb_log2_size_y - g->min_tb_log2_size_y;
    /* The address grid covers whole trees, including a partial tree at the
     * picture edge. The shift is non-negative because the tree is at least
     * as large as the minimum transform block. */
    grid_w = g->pic_width_in_ctbs_y << diff;
    grid_h = g->pic_height_in_ctbs_y << diff;
    if (x_tb < 0 || y_tb < 0 || x_tb >= grid_w || y_tb >= grid_h) {
        return -1;
    }

    /* Which tree contains this 4x4. Both shifts are non-negative. */
    ctb_x = (x_tb << g->min_tb_log2_size_y) >> g->ctb_log2_size_y;
    ctb_y = (y_tb << g->min_tb_log2_size_y) >> g->ctb_log2_size_y;
    ctb_addr_rs = g->pic_width_in_ctbs_y * ctb_y + ctb_x;

    /* Version 0 has one tile. Clause 6.5.1 would convert raster order to
     * tile-scan order here. With one tile that conversion is the identity.
     * A second tile replaces this assignment. The interleave below stays. */
    ctb_addr_ts = ctb_addr_rs;

    /* Leave room for every 4x4 inside this tree. diff * 2 is the log2 of
     * the number of 4x4s in the tree. */
    addr = ctb_addr_ts << (diff * 2);

    /* Interleave x and y. Bit i of x adds m*m. Bit i of y adds 2*m*m.
     * That is quadtree order: x is the even step, y is the odd step. */
    p = 0;
    for (i = 0; i < diff; i++) {
        m = 1 << i;
        if ((x_tb & m) != 0) {
            p = p + (m * m);
        }
        if ((y_tb & m) != 0) {
            p = p + (2 * m * m);
        }
    }
    return addr + p;
}

/* Decide whether one reconstructed luma sample may be read.
 * Slice and tile membership are arguments because this file does not own
 * the slice map. The caller passes 1 only when membership matches.
 */
int32_t hevc_neighbor_available(const HevcPictureGeometry *g,
                                int32_t x_curr,
                                int32_t y_curr,
                                int32_t x_nb,
                                int32_t y_nb,
                                int32_t same_slice,
                                int32_t same_tile)
{
    int32_t curr_addr;
    int32_t nb_addr;

    if (g == NULL || g->min_tb_size_y <= 0) {
        return -1;
    }
    /* The current sample is the caller's block. A position outside the
     * decoded picture means the caller is wrong, which is not the same as
     * an unavailable neighbour. */
    if (x_curr < 0 || y_curr < 0 ||
        x_curr >= g->pic_width_in_luma_samples ||
        y_curr >= g->pic_height_in_luma_samples) {
        return -1;
    }
    if (x_nb < 0 || y_nb < 0 ||
        x_nb >= g->pic_width_in_luma_samples ||
        y_nb >= g->pic_height_in_luma_samples) {
        return 0;
    }
    if (same_slice == 0 || same_tile == 0) {
        return 0;
    }

    /* The address unit is the minimum transform block. The shifts divide
     * by that size. Coordinates here are inside the picture, so they are
     * non-negative and the shift matches the specification. */
    curr_addr = hevc_min_tb_addr_zs(g,
                                    x_curr >> g->min_tb_log2_size_y,
                                    y_curr >> g->min_tb_log2_size_y);
    nb_addr = hevc_min_tb_addr_zs(g,
                                  x_nb >> g->min_tb_log2_size_y,
                                  y_nb >> g->min_tb_log2_size_y);
    if (curr_addr < 0 || nb_addr < 0) {
        return -1;
    }

    /* A later address has not been reconstructed. An equal address is the
     * same 4x4 as the current sample, so it is available. */
    if (nb_addr > curr_addr) {
        return 0;
    }
    return 1;
}
