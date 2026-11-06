#ifndef HEVC_LOOP_DEBLOCK_H
#define HEVC_LOOP_DEBLOCK_H

#include "hevc/status.h"

#include <stdint.h>

/* One 4-sample-tall deblock segment. Clause 8.7.2.5.
 * p[line][i] is the sample i steps away from the edge on the p side.
 * i = 0 touches the edge. q is the same shape on the other side.
 * The function writes the filtered samples back into p and q.
 * qp_p and qp_q are the luma QPs of the two coding units.
 * Offsets are the slice div2 syntax values. Pass 0 when the slice sends none.
 */

/* Luma decision and filter for boundary strength bS. bS 0 leaves the samples. */
HevcStatus hevc_deblock_luma_segment(int32_t p[4][4],
                                     int32_t q[4][4],
                                     int32_t qp_p,
                                     int32_t qp_q,
                                     int32_t b_s,
                                     int32_t beta_offset_div2,
                                     int32_t tc_offset_div2,
                                     int32_t bit_depth);

/* Chroma filter. Only the samples that touch the edge change.
 * c_qp_pic_offset is pps_cb_qp_offset or pps_cr_qp_offset.
 * 4:2:0 maps the average luma QP through Table 8-10.
 */
HevcStatus hevc_deblock_chroma_segment(int32_t p0[4],
                                       int32_t p1[4],
                                       int32_t q0[4],
                                       int32_t q1[4],
                                       int32_t qp_p,
                                       int32_t qp_q,
                                       int32_t c_qp_pic_offset,
                                       int32_t tc_offset_div2,
                                       int32_t bit_depth);

#endif
