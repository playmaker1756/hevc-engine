#ifndef HEVC_PICTURE_NEIGHBOR_AVAILABILITY_H
#define HEVC_PICTURE_NEIGHBOR_AVAILABILITY_H

#include "hevc/picture/geometry.h"

#include <stdint.h>

/* Z-scan address of the minimum transform block at (x_tb, y_tb).
 * Coordinates are in minimum-transform-block units, not luma samples.
 * Returns the address, or -1 when the block is outside the coding-tree grid
 * or the geometry has not been derived.
 */
int32_t hevc_min_tb_addr_zs(const HevcPictureGeometry *g, int32_t x_tb, int32_t y_tb);

/* Clause 6.4.1 for one luma sample.
 * Returns 1 when the neighbour may be read, 0 when it may not, and -1 when
 * the current location or the geometry cannot be used.
 * same_slice is 1 when the neighbour is in the current slice. same_tile is 1
 * when it is in the current tile. Any other value means it is not.
 * An address equal to the current address is available. Only a later address
 * is rejected.
 */
int32_t hevc_neighbor_available(const HevcPictureGeometry *g,
                                int32_t x_curr,
                                int32_t y_curr,
                                int32_t x_nb,
                                int32_t y_nb,
                                int32_t same_slice,
                                int32_t same_tile);

#endif
