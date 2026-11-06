#ifndef HEVC_COMPARE_H
#define HEVC_COMPARE_H

#include "hevc/status.h"

#include <stdint.h>

/* Report the first index where two int32 sequences differ.
 * On HEVC_OK, *first_index is -1.
 * On HEVC_MISMATCH, *first_index is the smallest i with actual[i] != expected[i].
 * Indices after that are not visited.
 */
HevcStatus hevc_compare_i32(const int32_t *actual,
                            const int32_t *expected,
                            int32_t count,
                            int32_t *first_index);

#endif
