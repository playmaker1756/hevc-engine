#include "hevc/loop/deblock.h"

#include "hevc/common/integer_spec.h"

/* Table 8-12, beta' for Q = 0..51. Q = 52 and 53 are not used for beta. */
static const int32_t k_beta_prime[52] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
    20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42,
    44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64
};

/* Table 8-12, tC' for Q = 0..53. */
static const int32_t k_tc_prime[54] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,
    5, 5, 6, 6, 7, 8, 9, 10, 11, 13, 14, 16, 18, 20, 22, 24
};

/* 2 * value without shifting a negative signed value.
 * The caller has already limited value to -6..6. */
static int32_t twice(int32_t value)
{
    if (value >= 0) {
        return value << 1;
    }
    return -((-value) << 1);
}

static int32_t abs_i32(int32_t value)
{
    if (value < 0) {
        return -value;
    }
    return value;
}

/* 1 when every sample sits in 0..max_sample. A sample outside that range
 * can overflow the later additions. */
static int32_t samples_in_range(const int32_t *samples, int32_t count, int32_t max_sample)
{
    int32_t i;

    for (i = 0; i < count; i++) {
        if (samples[i] < 0 || samples[i] > max_sample) {
            return 0;
        }
    }
    return 1;
}

/* Table 8-10 for 4:2:0. Below 30 the chroma QP equals the index.
 * From 30 to 43 the table bends. Above 43 the index drops by 6. */
static int32_t chroma_qp(int32_t q_pi)
{
    static const int32_t k_bend[14] = {
        29, 30, 31, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37
    };

    if (q_pi < 30) {
        return q_pi;
    }
    if (q_pi > 43) {
        return q_pi - 6;
    }
    return k_bend[q_pi - 30];
}

static int32_t sample_delta(int32_t left, int32_t right)
{
    int32_t diff;

    /* 4 * (q0 - p0) without shifting a negative difference. */
    diff = right - left;
    if (diff >= 0) {
        return diff << 2;
    }
    return -((-diff) << 2);
}

/* Filter one luma segment of four lines. */
HevcStatus hevc_deblock_luma_segment(int32_t p[4][4],
                                     int32_t q[4][4],
                                     int32_t qp_p,
                                     int32_t qp_q,
                                     int32_t b_s,
                                     int32_t beta_offset_div2,
                                     int32_t tc_offset_div2,
                                     int32_t bit_depth)
{
    int32_t q_pl;
    int32_t q_beta;
    int32_t q_tc;
    int32_t beta;
    int32_t tc;
    int32_t dp0;
    int32_t dp3;
    int32_t dq0;
    int32_t dq3;
    int32_t dpq0;
    int32_t dpq3;
    int32_t dp;
    int32_t dq;
    int32_t d;
    int32_t d_e;
    int32_t d_ep;
    int32_t d_eq;
    int32_t side_limit;
    int32_t k;

    if (p == NULL || q == NULL) {
        return HEVC_ERR_ARG;
    }
    if (bit_depth < 8 || bit_depth > 16 || b_s < 0 || b_s > 2) {
        return HEVC_ERR_RANGE;
    }
    if (qp_p < 0 || qp_p > 51 || qp_q < 0 || qp_q > 51) {
        return HEVC_ERR_RANGE;
    }
    if (beta_offset_div2 < -6 || beta_offset_div2 > 6 ||
        tc_offset_div2 < -6 || tc_offset_div2 > 6) {
        return HEVC_ERR_RANGE;
    }
    {
        int32_t max_sample;

        max_sample = (1 << bit_depth) - 1;
        if (samples_in_range(&p[0][0], 16, max_sample) == 0 ||
            samples_in_range(&q[0][0], 16, max_sample) == 0) {
            return HEVC_ERR_RANGE;
        }
    }
    if (b_s == 0) {
        return HEVC_OK;
    }

    q_pl = (qp_q + qp_p + 1) >> 1;
    q_beta = hevc_clip3(0, 51, q_pl + twice(beta_offset_div2));
    q_tc = hevc_clip3(0, 53, q_pl + 2 * (b_s - 1) + twice(tc_offset_div2));
    /* At 8 bits the table value is the threshold. Each extra bit doubles it. */
    beta = k_beta_prime[q_beta] << (bit_depth - 8);
    tc = k_tc_prime[q_tc] << (bit_depth - 8);

    dp0 = abs_i32(p[0][2] - 2 * p[0][1] + p[0][0]);
    dp3 = abs_i32(p[3][2] - 2 * p[3][1] + p[3][0]);
    dq0 = abs_i32(q[0][2] - 2 * q[0][1] + q[0][0]);
    dq3 = abs_i32(q[3][2] - 2 * q[3][1] + q[3][0]);
    dpq0 = dp0 + dq0;
    dpq3 = dp3 + dq3;
    dp = dp0 + dp3;
    dq = dq0 + dq3;
    d = dpq0 + dpq3;

    d_e = 0;
    d_ep = 0;
    d_eq = 0;
    if (d < beta) {
        int32_t d_sam0;
        int32_t d_sam3;

        d_e = 1;
        /* Lines 0 and 3 vote. Lines 1 and 2 are filtered with that vote. */
        d_sam0 = 0;
        if (2 * dpq0 < (beta >> 2) &&
            abs_i32(p[0][3] - p[0][0]) + abs_i32(q[0][0] - q[0][3]) < (beta >> 3) &&
            abs_i32(p[0][0] - q[0][0]) < ((5 * tc + 1) >> 1)) {
            d_sam0 = 1;
        }
        d_sam3 = 0;
        if (2 * dpq3 < (beta >> 2) &&
            abs_i32(p[3][3] - p[3][0]) + abs_i32(q[3][0] - q[3][3]) < (beta >> 3) &&
            abs_i32(p[3][0] - q[3][0]) < ((5 * tc + 1) >> 1)) {
            d_sam3 = 1;
        }
        if (d_sam0 == 1 && d_sam3 == 1) {
            d_e = 2;
        }
        side_limit = (beta + (beta >> 1)) >> 3;
        if (dp < side_limit) {
            d_ep = 1;
        }
        if (dq < side_limit) {
            d_eq = 1;
        }
    }

    if (d_e == 0) {
        return HEVC_OK;
    }

    for (k = 0; k < 4; k++) {
        if (d_e == 2) {
            int32_t p0;
            int32_t p1;
            int32_t p2;
            int32_t p3;
            int32_t q0;
            int32_t q1;
            int32_t q2;
            int32_t q3;

            /* Every new sample is a mix of the original line. Reading the
             * array after the first write would mix old and new values. */
            p0 = p[k][0];
            p1 = p[k][1];
            p2 = p[k][2];
            p3 = p[k][3];
            q0 = q[k][0];
            q1 = q[k][1];
            q2 = q[k][2];
            q3 = q[k][3];
            p[k][0] = hevc_clip3(p0 - 2 * tc, p0 + 2 * tc, (p2 + 2 * p1 + 2 * p0 + 2 * q0 + q1 + 4) >> 3);
            p[k][1] = hevc_clip3(p1 - 2 * tc, p1 + 2 * tc, (p2 + p1 + p0 + q0 + 2) >> 2);
            p[k][2] = hevc_clip3(p2 - 2 * tc, p2 + 2 * tc, (2 * p3 + 3 * p2 + p1 + p0 + q0 + 4) >> 3);
            q[k][0] = hevc_clip3(q0 - 2 * tc, q0 + 2 * tc, (p1 + 2 * p0 + 2 * q0 + 2 * q1 + q2 + 4) >> 3);
            q[k][1] = hevc_clip3(q1 - 2 * tc, q1 + 2 * tc, (p0 + q0 + q1 + q2 + 2) >> 2);
            q[k][2] = hevc_clip3(q2 - 2 * tc, q2 + 2 * tc, (p0 + q0 + q1 + 3 * q2 + 2 * q3 + 4) >> 3);
        } else {
            int32_t delta;
            int32_t p0;
            int32_t q0;

            /* The original p0 and q0 are both read. Store them before writing. */
            p0 = p[k][0];
            q0 = q[k][0];
            delta = hevc_floor_div_pow2(9 * (q0 - p0) - 3 * (q[k][1] - p[k][1]) + 8, 4);
            if (abs_i32(delta) < tc * 10) {
                int32_t delta_p;
                int32_t delta_q;

                delta = hevc_clip3(-tc, tc, delta);
                p[k][0] = hevc_clip3(0, (1 << bit_depth) - 1, p0 + delta);
                q[k][0] = hevc_clip3(0, (1 << bit_depth) - 1, q0 - delta);
                if (d_ep == 1) {
                    delta_p = hevc_floor_div_pow2(((p[k][2] + p0 + 1) >> 1) - p[k][1] + delta, 1);
                    delta_p = hevc_clip3(-(tc >> 1), tc >> 1, delta_p);
                    p[k][1] = hevc_clip3(0, (1 << bit_depth) - 1, p[k][1] + delta_p);
                }
                if (d_eq == 1) {
                    delta_q = hevc_floor_div_pow2(((q[k][2] + q0 + 1) >> 1) - q[k][1] - delta, 1);
                    delta_q = hevc_clip3(-(tc >> 1), tc >> 1, delta_q);
                    q[k][1] = hevc_clip3(0, (1 << bit_depth) - 1, q[k][1] + delta_q);
                }
            }
        }
    }
    return HEVC_OK;
}

/* Filter the chroma samples that touch the edge. */
HevcStatus hevc_deblock_chroma_segment(int32_t p0[4],
                                       int32_t p1[4],
                                       int32_t q0[4],
                                       int32_t q1[4],
                                       int32_t qp_p,
                                       int32_t qp_q,
                                       int32_t c_qp_pic_offset,
                                       int32_t tc_offset_div2,
                                       int32_t bit_depth)
{
    int32_t q_pi;
    int32_t qp_c;
    int32_t q_tc;
    int32_t tc;
    int32_t k;
    int32_t max_sample;

    if (p0 == NULL || p1 == NULL || q0 == NULL || q1 == NULL) {
        return HEVC_ERR_ARG;
    }
    if (bit_depth < 8 || bit_depth > 16) {
        return HEVC_ERR_RANGE;
    }
    if (qp_p < 0 || qp_p > 51 || qp_q < 0 || qp_q > 51) {
        return HEVC_ERR_RANGE;
    }
    if (c_qp_pic_offset < -12 || c_qp_pic_offset > 12 ||
        tc_offset_div2 < -6 || tc_offset_div2 > 6) {
        return HEVC_ERR_RANGE;
    }
    max_sample = (1 << bit_depth) - 1;
    if (samples_in_range(p0, 4, max_sample) == 0 ||
        samples_in_range(p1, 4, max_sample) == 0 ||
        samples_in_range(q0, 4, max_sample) == 0 ||
        samples_in_range(q1, 4, max_sample) == 0) {
        return HEVC_ERR_RANGE;
    }

    q_pi = ((qp_q + qp_p + 1) >> 1) + c_qp_pic_offset;
    qp_c = chroma_qp(q_pi);
    q_tc = hevc_clip3(0, 53, qp_c + 2 + twice(tc_offset_div2));
    tc = k_tc_prime[q_tc] << (bit_depth - 8);

    for (k = 0; k < 4; k++) {
        int32_t delta;

        delta = hevc_floor_div_pow2(sample_delta(p0[k], q0[k]) + p1[k] - q1[k] + 4, 3);
        delta = hevc_clip3(-tc, tc, delta);
        p0[k] = hevc_clip3(0, max_sample, p0[k] + delta);
        q0[k] = hevc_clip3(0, max_sample, q0[k] - delta);
    }
    return HEVC_OK;
}
