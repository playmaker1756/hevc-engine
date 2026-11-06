#include "hevc/intra/mode.h"

#include <stddef.h>

/* Three most-probable luma modes from the two neighbour modes. */
HevcStatus hevc_intra_mpm_list(int32_t cand_a, int32_t cand_b, int32_t list[3])
{
    if (list == NULL) {
        return HEVC_ERR_ARG;
    }
    if (cand_a < 0 || cand_a > 34 || cand_b < 0 || cand_b > 34) {
        return HEVC_ERR_RANGE;
    }

    if (cand_a == cand_b) {
        if (cand_a < 2) {
            /* Both neighbours are planar or DC, or both were forced to DC. */
            list[0] = 0;
            list[1] = 1;
            list[2] = 26;
        } else {
            list[0] = cand_a;
            /* The two angular neighbours of this mode, wrapped inside 2..33. */
            list[1] = 2 + ((cand_a + 29) % 32);
            list[2] = 2 + ((cand_a - 2 + 1) % 32);
        }
        return HEVC_OK;
    }

    list[0] = cand_a;
    list[1] = cand_b;
    if (list[0] != 0 && list[1] != 0) {
        list[2] = 0;
    } else if (list[0] != 1 && list[1] != 1) {
        list[2] = 1;
    } else {
        list[2] = 26;
    }
    return HEVC_OK;
}

/* Turn a remainder into a mode that is not one of the three candidates. */
HevcStatus hevc_intra_mode_from_remainder(const int32_t list[3], int32_t remainder, int32_t *mode)
{
    int32_t sorted[3];
    int32_t i;
    int32_t value;

    if (list == NULL || mode == NULL) {
        return HEVC_ERR_ARG;
    }
    if (remainder < 0 || remainder > 31) {
        return HEVC_ERR_RANGE;
    }

    sorted[0] = list[0];
    sorted[1] = list[1];
    sorted[2] = list[2];
    /* Three compare-and-swap steps. The specification sorts before scanning. */
    if (sorted[0] > sorted[1]) {
        value = sorted[0];
        sorted[0] = sorted[1];
        sorted[1] = value;
    }
    if (sorted[0] > sorted[2]) {
        value = sorted[0];
        sorted[0] = sorted[2];
        sorted[2] = value;
    }
    if (sorted[1] > sorted[2]) {
        value = sorted[1];
        sorted[1] = sorted[2];
        sorted[2] = value;
    }

    value = remainder;
    for (i = 0; i < 3; i++) {
        if (value >= sorted[i]) {
            value = value + 1;
        }
    }
    *mode = value;
    return HEVC_OK;
}

/* Chroma mode from Table 8-2. */
HevcStatus hevc_chroma_intra_mode(int32_t luma_mode, int32_t chroma_syntax, int32_t *mode)
{
    /* Columns are luma modes 0, 26, 10, 1, and every other mode.
     * Rows are intra_chroma_pred_mode 0..4. The first column of the printed
     * table is the row index, not a mode.
     */
    static const int32_t k_table[5][5] = {
        {34, 0, 0, 0, 0},
        {26, 34, 26, 26, 26},
        {10, 10, 34, 10, 10},
        {1, 1, 1, 34, 1},
        {0, 26, 10, 1, -1},
    };
    int32_t column;

    if (mode == NULL) {
        return HEVC_ERR_ARG;
    }
    if (luma_mode < 0 || luma_mode > 34 || chroma_syntax < 0 || chroma_syntax > 4) {
        return HEVC_ERR_RANGE;
    }

    if (luma_mode == 0) {
        column = 0;
    } else if (luma_mode == 26) {
        column = 1;
    } else if (luma_mode == 10) {
        column = 2;
    } else if (luma_mode == 1) {
        column = 3;
    } else {
        column = 4;
    }

    if (chroma_syntax == 4 && column == 4) {
        *mode = luma_mode;
    } else {
        *mode = k_table[chroma_syntax][column];
    }
    return HEVC_OK;
}
