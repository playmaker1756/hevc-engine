#include "compare_test.h"

#include "expect.h"
#include "hevc/compare.h"

#include <stddef.h>

/* The checker itself: first difference, a full match, and calls it must reject. */
void hevc_test_compare(void)
{
    const int32_t actual[4] = {1, 2, 9, 4};
    const int32_t expected[4] = {1, 2, 3, 4};
    const int32_t same[3] = {8, 8, 8};
    int32_t index;

    index = 99;
    hevc_test_expect_status("compare", "first mismatch",
                            hevc_compare_i32(actual, expected, 4, &index), HEVC_MISMATCH);
    hevc_test_expect_i32("compare", "mismatch index", index, 2);

    index = 99;
    hevc_test_expect_status("compare", "equal arrays",
                            hevc_compare_i32(same, same, 3, &index), HEVC_OK);
    hevc_test_expect_i32("compare", "equal index", index, -1);

    hevc_test_expect_status("compare", "null buffer",
                            hevc_compare_i32(NULL, expected, 4, &index), HEVC_ERR_ARG);
    hevc_test_expect_status("compare", "negative count",
                            hevc_compare_i32(actual, expected, -1, &index), HEVC_ERR_ARG);
    hevc_test_expect_status("compare", "null index",
                            hevc_compare_i32(actual, expected, 4, NULL), HEVC_ERR_ARG);
    hevc_test_expect_status("compare", "empty range",
                            hevc_compare_i32(NULL, NULL, 0, &index), HEVC_OK);
}
