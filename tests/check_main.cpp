#include "hevc/check.h"
#include "hevc/geometry.h"

#include <stdio.h>

static int g_failures;

static void report(const char *component, const char *case_name, int index, int expected, int actual)
{
    printf("%s / %s: index %d expected %d actual %d\n",
           component, case_name, index, expected, actual);
    g_failures++;
}

static void expect_status(const char *component, const char *case_name, HevcStatus got, HevcStatus want)
{
    int index = -1;
    int actual = (int)got;
    int expected = (int)want;
    HevcStatus st = hevc_compare_ints(&actual, &expected, 1, &index);
    if (st != HEVC_OK) {
        report(component, case_name, index, expected, actual);
    }
}

static void expect_int(const char *component, const char *case_name, int actual, int expected)
{
    int index = -1;
    HevcStatus st = hevc_compare_ints(&actual, &expected, 1, &index);
    if (st != HEVC_OK) {
        report(component, case_name, index, expected, actual);
    }
}

static void test_compare(void)
{
    const int actual[4] = {1, 2, 9, 4};
    const int expected[4] = {1, 2, 3, 4};
    const int same[3] = {8, 8, 8};
    int index = 99;

    expect_status("compare", "first mismatch is index 2",
                  hevc_compare_ints(actual, expected, 4, &index), HEVC_MISMATCH);
    expect_int("compare", "mismatch index", index, 2);

    index = 99;
    expect_status("compare", "equal arrays",
                  hevc_compare_ints(same, same, 3, &index), HEVC_OK);
    expect_int("compare", "equal index is -1", index, -1);

    expect_status("compare", "null buffer",
                  hevc_compare_ints(0, expected, 4, &index), HEVC_ERR_ARG);
    expect_status("compare", "negative count",
                  hevc_compare_ints(actual, expected, -1, &index), HEVC_ERR_ARG);
    expect_status("compare", "null index",
                  hevc_compare_ints(actual, expected, 4, 0), HEVC_ERR_ARG);
    expect_status("compare", "empty range is equal",
                  hevc_compare_ints(0, 0, 0, &index), HEVC_OK);
}

static void fill_version0(HevcGeometry *g, int width, int height)
{
    *g = HevcGeometry{};
    g->pic_width_in_luma_samples = width;
    g->pic_height_in_luma_samples = height;
    g->sub_width_c = 2;
    g->sub_height_c = 2;
    g->bit_depth_y = 8;
    g->bit_depth_c = 8;
    g->min_cb_log2_size_y = 3;
    g->ctb_log2_size_y = 6;
}

static void test_geometry(void)
{
    HevcGeometry g;

    fill_version0(&g, 1920, 1080);
    expect_status("geometry", "1920x1080 derives", hevc_geometry_derive(&g), HEVC_OK);
    expect_int("geometry", "ctb size", g.ctb_size_y, 64);
    expect_int("geometry", "min cb size", g.min_cb_size_y, 8);
    expect_int("geometry", "1920 covers 30 ctbs, not 16 from truncating division", g.pic_width_in_ctbs_y, 30);
    expect_int("geometry", "1080 covers 17 ctbs, not 16", g.pic_height_in_ctbs_y, 17);
    expect_int("geometry", "chroma width", g.pic_width_in_samples_c, 960);
    expect_int("geometry", "chroma height", g.pic_height_in_samples_c, 540);
    expect_int("geometry", "zero crop x0", g.crop_x0, 0);
    expect_int("geometry", "zero crop y0", g.crop_y0, 0);
    expect_int("geometry", "zero crop x1", g.crop_x1, 1919);
    expect_int("geometry", "zero crop y1", g.crop_y1, 1079);
    expect_int("geometry", "output width", g.crop_x1 - g.crop_x0 + 1, 1920);
    expect_int("geometry", "output height", g.crop_y1 - g.crop_y0 + 1, 1080);

    fill_version0(&g, 1920, 1088);
    g.conf_win_bottom_offset = 4;
    expect_status("geometry", "padded 1088 derives", hevc_geometry_derive(&g), HEVC_OK);
    expect_int("geometry", "1088 is 17 full ctbs", g.pic_height_in_ctbs_y, 17);
    expect_int("geometry", "bottom offset 4 removes 8 luma rows", g.crop_y1, 1079);
    expect_int("geometry", "output height after pad", g.crop_y1 - g.crop_y0 + 1, 1080);

    fill_version0(&g, 4, 8);
    expect_status("geometry", "width below min cb is rejected", hevc_geometry_derive(&g), HEVC_ERR_RANGE);

    fill_version0(&g, 64, 64);
    g.conf_win_left_offset = 16;
    g.conf_win_right_offset = 16;
    expect_status("geometry", "crop covering the whole picture is rejected",
                  hevc_geometry_derive(&g), HEVC_ERR_RANGE);

    expect_status("geometry", "null geometry", hevc_geometry_derive(0), HEVC_ERR_ARG);
}

int main(void)
{
    test_compare();
    test_geometry();
    if (g_failures != 0) {
        printf("%d checks failed\n", g_failures);
        return 1;
    }
    printf("all checks passed\n");
    return 0;
}
