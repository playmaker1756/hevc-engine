#ifndef HEVC_BITS_RBSP_H
#define HEVC_BITS_RBSP_H

#include "hevc/status.h"

#include <stdint.h>

/* Clause 7.3.1.1. Copy a NAL unit into an RBSP.
 * The two header bytes stay out of the RBSP. A 00 00 03 sequence drops the 03
 * and keeps the two zeros. rbsp_size is the number of bytes written.
 */
HevcStatus hevc_rbsp_from_nal(const uint8_t *nal,
                              int32_t nal_size,
                              uint8_t *rbsp,
                              int32_t rbsp_cap,
                              int32_t *rbsp_size);

#endif
