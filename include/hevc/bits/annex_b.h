#ifndef HEVC_BITS_ANNEX_B_H
#define HEVC_BITS_ANNEX_B_H

#include "hevc/status.h"

#include <stdint.h>

/* Clause B.3. Pull the next NAL unit out of an Annex B byte stream.
 * pos is the byte index to start from. On success it advances to the first
 * byte after this NAL unit. payload_offset is an index into bytes; the
 * payload is not copied.
 * The search follows B.3: the first unit is aligned on 00 00 00 01. A later
 * unit may use the three-byte prefix 00 00 01 when the extra zero is absent.
 */
HevcStatus hevc_annexb_next_nal(const uint8_t *bytes,
                                int32_t size,
                                int32_t *pos,
                                int32_t *payload_offset,
                                int32_t *payload_size);

#endif
