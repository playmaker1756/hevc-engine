#ifndef HEVC_TEST_EXPECT_H
#define HEVC_TEST_EXPECT_H

#include "hevc/status.h"

#include <stdint.h>

/* Compare one int32 through hevc_compare_i32 and record a mismatch. */
void hevc_test_expect_i32(const char *problem, const char *what, int32_t actual, int32_t expected);

/* Compare a status code the same way. The code is stored as an int32 so the
 * integer checker is the only comparison. */
void hevc_test_expect_status(const char *problem, const char *what, HevcStatus actual, HevcStatus expected);

/* Compare count int32 values. The report index is the first difference. */
void hevc_test_expect_row(const char *problem,
                          const char *what,
                          const int32_t *actual,
                          const int32_t *expected,
                          int32_t count);

/* How many expects have failed since the process started. */
int32_t hevc_test_failure_count(void);

#endif
