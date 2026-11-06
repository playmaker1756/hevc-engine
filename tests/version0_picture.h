#ifndef HEVC_TEST_VERSION0_PICTURE_H
#define HEVC_TEST_VERSION0_PICTURE_H

#include "hevc/picture/geometry.h"

/* Write the version-0 inputs: 8-bit 4:2:0, 8-sample minimum coding block,
 * 64-sample coding tree, 4-sample minimum transform block, zero crop.
 * Derived fields are cleared so a skipped derive cannot look successful.
 */
void hevc_test_fill_version0(HevcPictureGeometry *g, int32_t width, int32_t height);

#endif
