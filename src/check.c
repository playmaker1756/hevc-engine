#include "hevc/check.h"

HevcStatus hevc_compare_ints(const int *actual,
                             const int *expected,
                             int count,
                             int *first_index)
{
    int i;

    if (first_index == 0 || count < 0 || (count > 0 && (actual == 0 || expected == 0))) {
        return HEVC_ERR_ARG;
    }

    for (i = 0; i < count; i++) {
        if (actual[i] != expected[i]) {
            *first_index = i;
            return HEVC_MISMATCH;
        }
    }

    *first_index = -1;
    return HEVC_OK;
}
