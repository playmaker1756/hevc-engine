#ifndef HEVC_CHECK_H
#define HEVC_CHECK_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum HevcStatus {
    HEVC_OK = 0,
    HEVC_ERR_ARG = 1,
    HEVC_ERR_RANGE = 2,
    HEVC_MISMATCH = 3
} HevcStatus;

/*
 * Compare count integers. On HEVC_OK, *first_index is -1.
 * On HEVC_MISMATCH, *first_index is the smallest i with actual[i] != expected[i].
 * Later differences are not reported. No tolerance.
 */
HevcStatus hevc_compare_ints(const int *actual,
                             const int *expected,
                             int count,
                             int *first_index);

#ifdef __cplusplus
}
#endif

#endif
