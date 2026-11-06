#include "hevc/bits/annex_b.h"

#include <stddef.h>

/* True when the next three bytes are a start-code prefix. */
static int32_t starts_with_3(const uint8_t *bytes, int32_t size, int32_t pos)
{
    if (pos < 0 || size < 3 || pos > size - 3) {
        return 0;
    }
    if (bytes[pos] == 0 && bytes[pos + 1] == 0 && bytes[pos + 2] == 1) {
        return 1;
    }
    return 0;
}

/* True when the next four bytes are 00 00 00 01. */
static int32_t starts_with_4(const uint8_t *bytes, int32_t size, int32_t pos)
{
    if (pos < 0 || size < 4 || pos > size - 4) {
        return 0;
    }
    if (bytes[pos] == 0 && bytes[pos + 1] == 0 && bytes[pos + 2] == 0 && bytes[pos + 3] == 1) {
        return 1;
    }
    return 0;
}

/* Extract one NAL payload and advance pos past it. */
HevcStatus hevc_annexb_next_nal(const uint8_t *bytes,
                                int32_t size,
                                int32_t *pos,
                                int32_t *payload_offset,
                                int32_t *payload_size)
{
    int32_t cursor;
    int32_t start;
    int32_t end;

    if (bytes == NULL || pos == NULL || payload_offset == NULL || payload_size == NULL) {
        return HEVC_ERR_ARG;
    }
    if (size < 0 || *pos < 0 || *pos > size) {
        return HEVC_ERR_RANGE;
    }

    cursor = *pos;
    /* A three-byte prefix is already a start code. Leading zeros are
     * discarded only until a four-byte start code, which is how B.3 finds
     * the first unit and how a later unit carries the extra zero_byte. */
    if (starts_with_3(bytes, size, cursor) == 0) {
        while (starts_with_4(bytes, size, cursor) == 0) {
            if (cursor >= size) {
                return HEVC_ERR_RANGE;
            }
            if (bytes[cursor] != 0) {
                return HEVC_ERR_RANGE;
            }
            cursor = cursor + 1;
        }
    }

    /* Drop the extra zero when the prefix is four bytes. */
    if (starts_with_4(bytes, size, cursor) == 1) {
        cursor = cursor + 1;
    }
    if (starts_with_3(bytes, size, cursor) == 0) {
        return HEVC_ERR_RANGE;
    }
    /* Drop 00 00 01. The payload starts on the following byte. */
    cursor = cursor + 3;
    start = cursor;

    /* The payload ends at the next 00 00 00, the next 00 00 01, or the
     * end of the buffer. */
    end = start;
    while (end < size) {
        if (starts_with_3(bytes, size, end) == 1 || starts_with_4(bytes, size, end) == 1) {
            break;
        }
        /* A run of three zeros is also a boundary, even without the one. */
        if (size >= 3 && end <= size - 3 && bytes[end] == 0 && bytes[end + 1] == 0 &&
            bytes[end + 2] == 0) {
            break;
        }
        end = end + 1;
    }

    *payload_offset = start;
    *payload_size = end - start;
    *pos = end;
    return HEVC_OK;
}
