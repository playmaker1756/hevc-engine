#include "hevc/bits/rbsp.h"

#include <stddef.h>

/* Remove emulation-prevention bytes from one NAL unit. */
HevcStatus hevc_rbsp_from_nal(const uint8_t *nal,
                              int32_t nal_size,
                              uint8_t *rbsp,
                              int32_t rbsp_cap,
                              int32_t *rbsp_size)
{
    int32_t i;
    int32_t out;

    if (nal == NULL || rbsp == NULL || rbsp_size == NULL) {
        return HEVC_ERR_ARG;
    }
    if (nal_size < 2 || rbsp_cap < 0) {
        return HEVC_ERR_RANGE;
    }

    /* i starts at 2 because the NAL header is not part of the RBSP. */
    out = 0;
    i = 2;
    while (i < nal_size) {
        if (i + 2 < nal_size && nal[i] == 0 && nal[i + 1] == 0 && nal[i + 2] == 3) {
            if (out + 2 > rbsp_cap) {
                return HEVC_ERR_RANGE;
            }
            rbsp[out] = nal[i];
            rbsp[out + 1] = nal[i + 1];
            out = out + 2;
            /* The 03 is the prevention byte. Skip it and the two zeros. */
            i = i + 3;
        } else {
            if (out + 1 > rbsp_cap) {
                return HEVC_ERR_RANGE;
            }
            rbsp[out] = nal[i];
            out = out + 1;
            i = i + 1;
        }
    }
    *rbsp_size = out;
    return HEVC_OK;
}
