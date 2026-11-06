#include "expect.h"

#include "hevc/compare.h"

#include <inttypes.h>
#include <stdio.h>

static int32_t g_failures;

/* Print one failed check. The index is the position hevc_compare_i32 stopped at. */
static void report(const char *problem, const char *what, int32_t index, int32_t expected, int32_t actual)
{
    printf("%s / %s: index %" PRId32 " expected %" PRId32 " actual %" PRId32 "\n",
           problem, what, index, expected, actual);
    g_failures = g_failures + 1;
}

void hevc_test_expect_i32(const char *problem, const char *what, int32_t actual, int32_t expected)
{
    int32_t actual_value;
    int32_t expected_value;
    int32_t index;
    HevcStatus status;

    actual_value = actual;
    expected_value = expected;
    index = -1;
    status = hevc_compare_i32(&actual_value, &expected_value, 1, &index);
    if (status != HEVC_OK) {
        report(problem, what, index, expected, actual);
    }
}

void hevc_test_expect_status(const char *problem, const char *what, HevcStatus actual, HevcStatus expected)
{
    hevc_test_expect_i32(problem, what, (int32_t)actual, (int32_t)expected);
}

void hevc_test_expect_row(const char *problem,
                          const char *what,
                          const int32_t *actual,
                          const int32_t *expected,
                          int32_t count)
{
    int32_t index;
    HevcStatus status;

    index = -1;
    status = hevc_compare_i32(actual, expected, count, &index);
    if (status != HEVC_OK) {
        report(problem, what, index, expected[index], actual[index]);
    }
}

int32_t hevc_test_failure_count(void)
{
    return g_failures;
}
