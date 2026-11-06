#ifndef HEVC_STATUS_H
#define HEVC_STATUS_H

/* How every component reports a result.
 * HEVC_OK: the operation produced its result.
 * HEVC_ERR_ARG: a pointer or a count cannot be used.
 * HEVC_ERR_RANGE: a value is outside the range this component accepts.
 * HEVC_MISMATCH: integer sequences differ. Only compare returns this.
 */
typedef enum HevcStatus {
    HEVC_OK = 0,
    HEVC_ERR_ARG = 1,
    HEVC_ERR_RANGE = 2,
    HEVC_MISMATCH = 3
} HevcStatus;

#endif
