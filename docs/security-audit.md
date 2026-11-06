# Security audit of the version 0 decoder

Four passes over every `.c` file under `src` and `tests`. The integer checks pass after the fixes.

1. `gcc -Wall -Wextra -Wpedantic -Wconversion -Wstrict-overflow=5 -fanalyzer -O2`
2. `gcc -Wall -Wextra -Wpedantic -Wshadow -Wundef -Wformat=2 -Wnull-dereference`
3. A line-by-line read of each function against C undefined behaviour and the clauses the function claims to implement.
4. The same analyzer command as pass 1, after the fixes.

Pass 2 is clean. Pass 4 repeats the `-Wstrict-overflow` notes below. `-fanalyzer` reported no out-of-bounds, leak, or null path. There is no `malloc` in this tree.

## What this decoder is exposed to

These are the C and specification pitfalls that apply to this code. A row is "fixed" when the function now rejects the bad value or computes the result in a wider type. "Contract" means a lying caller can still break it, because the array length is the caller's count. "Later" means the bug class appears when a parser exists. It is not in the functions written so far.

| Pitfall | Where it would show up | Status |
| --- | --- | --- |
| Signed overflow on a size or a sample sum | Tile map, crop, plane index, DC, planar, smooth, SAO, inverse scale | Fixed |
| Shift of a negative value, or a shift of 31 or more | CABAC, Exp-Golomb, neighbour address, SAO apply, PCM | Fixed where a bitstream or a caller can reach it |
| Index computed with a wrapping multiply | Reference gather `y * stride + x` | Fixed |
| Read of an unavailable neighbour | Gather indexes the plane only after availability is 1, and only when x and y are inside the stride | Fixed |
| Uninitialized stack used as a sample | Angular reference array | Fixed |
| Buffer write past a caller array | Tile scan if the column count is longer than the array | Contract. The function rejects a sum that wraps. It cannot see a short array. |
| `bit_pos + 1` wrapping on a huge buffer | Exp-Golomb cursor | Fixed. Buffers longer than 2^28 bytes are rejected. |
| CABAC range of 0 spinning in renormalization | Decision, bypass, terminate | Fixed. Range must be 2..510 and the offset must be below it. |
| Coefficient product wider than 64 bits | Inverse scale | Fixed. The result is clipped by sign. |
| Deblock difference of an out-of-range sample | Luma and chroma segments | Fixed. QP, offsets, and samples are bounded. |
| Start-code scan adding to a cursor near 2^31 | Annex B | Checked. The prefix tests subtract from the size. |
| Emulation-prevention length check adding to the output count | RBSP copy | Fixed |
| Right shift of a negative sample | SAO band | Fixed. A negative sample is category 0. |
| `sample + offset` wrapping before the clip | SAO apply | Fixed |
| Floor-divide by a shift above 30 | `hevc_floor_div_pow2` | Not reached. The function returns the input so it does not shift off the word. Callers pass 1..12. |
| Header, residual, and context-table parsing | Steps 7, 9, and 10 of the roadmap | Later. No slice parser exists, so there is no loop over a coded coefficient count yet. |
| Allocation of `width * height` | Not written | Later |

## Bugs caught, and the fix

### Tile-scan write past the map

`src/picture/coded_structure.c`. Column widths were added and multiplied in a signed 32-bit word. A wrapped product can equal a small `map_count`, and the loops then write past the two maps. Widths and heights now stop before the sum wraps. The product is a 64-bit multiply and must equal `map_count` before any store.

### Neighbour address shift

`src/picture/neighbor_availability.c`. `pic_width_in_ctbs_y << diff` and `x_tb << min_tb_log2` are undefined when the log2 is negative or the grid is wider than the word. The log2 is rejected outside 0..31, and the shift is rejected when the value does not fit. The raster address is formed in 64 bits. `hevc_neighbor_available` checks the minimum-transform log2 before it shifts the coordinates.

### PCM shift

`src/picture/coded_structure.c`. `pcm_sample << shift` is undefined when the sample is negative or wider than `PcmBitDepth`. The sample must be in `0 .. (1 << pcm_bit_depth) - 1` before the shift. `pcm_bit_depth` is already limited to 1..16.

### Crop offset

`src/picture/geometry.c`. `sub_width_c * (left + right)` wraps when an offset is near 2^31, and the wrapped crop looks legal. Both products are 64-bit and must be smaller than the picture.

### Bit cursor

`src/bits/exp_golomb.c`. `bit_pos` is a signed index. A buffer longer than 2^28 bytes makes `bit_pos + 1` undefined. Those sizes are rejected before a byte is read.

### Inverse scale

`src/residual/inverse_4x4.c`. `level * weight * scale`, shifted by `qp / 6`, can overflow a signed 64-bit product. A zero level stays zero. A product that cannot fit is clipped to the coefficient limit of the same sign. The negative path shifts the magnitude. The rounding add is refused when it cannot fit in 64 bits. The old path also narrowed the accumulator to 32 bits before the divide.

### Deblock inputs

`src/loop/deblock.c`. QP, the offset divisors, and the samples were unbounded, so `q0 - p0` and `2 * offset` could overflow. QP is 0..51, the luma offsets are -6..6, the chroma picture offset is -12..12, and every sample must lie in the bit depth. An unused `gate` local was removed. It was not part of the filter.

### Wavefront source and split flag

`src/picture/coded_structure.c`. `x0 + ctb_size` and `x0 + size` overflow at the top of a signed word. The wavefront function returns an error. The split test compares with `pic_width - size` after proving the picture is at least `size` wide. A coded flag that is not 0 or 1 is rejected.

### CABAC engine

`src/bits/cabac.c`. Decision, bypass, and terminate refuse a range outside 2..510 or an offset that is not strictly below the range. That keeps the following `<< 1` inside the word and stops a zero range from renormalizing forever.

### Annex B cursor

`src/bits/annex_b.c`. `pos + 3 > size` overflows when `pos` is near 2^31. The tests are `pos > size - 3` after `size` is at least 3. Pass 4 still prints `-Wstrict-overflow` on those lines. The `||` evaluates the subtraction only after `size >= 3`, so the subtraction itself does not wrap. The warning is the optimizer's assumption, not a path that executes.

### Reference gather

`src/intra/reference_gather.c`. The plane index was `y * stride + x` in 32 bits. A wrapped index reads off the front of the plane. The index is 64-bit, `x` must sit in `0 .. stride - 1`, and `y` must be non-negative. The block origin must be non-negative and at least 64 samples short of the top of a signed word, which covers a 32-sample block's reference arm. Chroma scale factors other than 1 and 2 are rejected.

### SAO

`src/loop/sao.c`. `center - neighbour` overflows when the samples sit at opposite ends of a signed word. The sign uses a 64-bit difference. `hevc_sao_apply` shifted `1 << bit_depth` for any bit depth and added the offset in 32 bits. Bit depth must be 8..16, and the sum is 64-bit before the clip. A negative sample is not shifted into a band.

### RBSP output count

`src/bits/rbsp.c`. `out + 2 > rbsp_cap` overflows when `out` is near 2^31. The check is `out > rbsp_cap - 2` after the capacity is at least 2. The same form is used for the one-byte copy.

### Angular reference

`src/intra/predict_angular.c`. `ref[97]` was uninitialized. A projection that missed a slot would copy stack contents into the prediction. Every slot is set to 0 first. Reads and writes must land on indices -32..64, and a borrowed-arm index must land inside the 2n samples of the other arm. The fractional mix is a 64-bit product.

### DC, planar, and smoothing

`src/intra/predict_dc.c`, `src/intra/predict_planar.c`, `src/intra/reference_smooth.c`. The weighted sums were 32-bit. Legal 8-bit and 16-bit samples fit, but a sample at the end of a signed word wraps the sum and the later shift is undefined. Those sums, the three-tap filter, the strong ramp, and the strong-smoothing bend are 64-bit. A DC or planar result that does not fit back into 32 bits is an error. In-range samples produce the same integers as before. The checks still pass, including DC 27, planar 5, and the strong ramp.

## What was reviewed and left unchanged

Exp-Golomb still refuses a prefix longer than 30 zeros, so `1 << leading` stays inside a positive signed word. The CABAC range table's smallest least-probable range is 2, so a decision does not hand renormalization a zero range. The cosine row index `i << (5 - log2(n))` stays inside 0..31 for n in {4, 8, 16, 32}. Transform-skip shifts the magnitude, not a negative signed value. `hevc_check` passes.
