#ifndef HEVC_BITS_CABAC_H
#define HEVC_BITS_CABAC_H

#include "hevc/bits/exp_golomb.h"
#include "hevc/status.h"

#include <stdint.h>

/* Arithmetic decoding engine. Clause 9.3.4.3.
 * range is ivlCurrRange. offset is ivlOffset. The cursor supplies the bits
 * that renormalization and bypass shift in.
 */
typedef struct HevcCabacEngine {
    int32_t range;
    int32_t offset;
    HevcBitCursor *cursor;
} HevcCabacEngine;

/* One context. p_state is pStateIdx. val_mps is the most probable symbol. */
typedef struct HevcCabacContext {
    int32_t p_state;
    int32_t val_mps;
} HevcCabacContext;

/* Clause 9.3.2.6. range starts at 510. offset is the next 9 bits. */
HevcStatus hevc_cabac_engine_init(HevcCabacEngine *engine, HevcBitCursor *cursor);

/* Equation 9-4 through 9-6. init_value is the 8-bit table entry. */
HevcStatus hevc_cabac_context_init(int32_t init_value, int32_t slice_qp, HevcCabacContext *ctx);

/* Equation 9-7. slice_type 2 is I, 1 is P, 0 is B, matching the specification.
 * Returns the initType, or -1 when the arguments are not a slice type.
 */
int32_t hevc_cabac_init_type(int32_t slice_type, int32_t cabac_init_flag);

/* Clause 9.3.4.3.2. Decode one context-coded bin and update the context. */
HevcStatus hevc_cabac_decision(HevcCabacEngine *engine, HevcCabacContext *ctx, int32_t *bin);

/* Clause 9.3.4.3.4. Decode one bypass bin. */
HevcStatus hevc_cabac_bypass(HevcCabacEngine *engine, int32_t *bin);

/* Clause 9.3.4.3.5. Decode the terminating bin. *finished is 1 when the
 * bin is 1 and the engine stops. */
HevcStatus hevc_cabac_terminate(HevcCabacEngine *engine, int32_t *bin, int32_t *finished);

#endif
