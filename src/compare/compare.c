#include "hevc/compare.h"

#include <stddef.h>

/* Find the earliest integer that differs.
 * Stopping there keeps the report on the first fault. Walking the rest
 * would spend cycles and would not change which index we return.
 */
HevcStatus hevc_compare_i32(const int32_t *actual,
                            const int32_t *expected,
                            int32_t count,
                            int32_t *first_index)
{
    int32_t i;

    /* The caller must be able to read the index we write. */
    if (first_index == NULL) {
        return HEVC_ERR_ARG;
    }
    /* A negative length is not a sequence. */
    if (count < 0) {
        return HEVC_ERR_ARG;
    }
    /* An empty sequence has nothing to disagree about. Null pointers are
     * allowed only in that case, because they are not dereferenced. */
    if (count == 0) {
        *first_index = -1;
        return HEVC_OK;
    }
    if (actual == NULL || expected == NULL) {
        return HEVC_ERR_ARG;
    }

    for (i = 0; i < count; i++) {
        /* Inequality is exact. There is no tolerance band. */
        if (actual[i] != expected[i]) {
            *first_index = i;
            return HEVC_MISMATCH;
        }
    }

    *first_index = -1;
    return HEVC_OK;
}
