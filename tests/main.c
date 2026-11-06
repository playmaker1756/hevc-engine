#include "cabac_transform_test.h"
#include "compare_test.h"
#include "derived_rest_test.h"
#include "intra_predict_test.h"
#include "neighbor_availability_test.h"
#include "picture_geometry_test.h"

#include "expect.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

/* Run every problem's checks. A non-zero status means at least one integer differed. */
int main(void)
{
    int32_t failures;

    hevc_test_compare();
    hevc_test_picture_geometry();
    hevc_test_neighbor_availability();
    hevc_test_intra_predict();
    hevc_test_derived_rest();
    hevc_test_cabac_transform();

    failures = hevc_test_failure_count();
    if (failures != 0) {
        printf("%" PRId32 " checks failed\n", failures);
        return 1;
    }
    printf("all checks passed\n");
    return 0;
}
