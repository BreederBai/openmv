/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (C) 2013-2024 OpenMV, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * SIMD abstraction.
 */
#include <arm_math.h>
#include <cmsis_extension.h>

#if (__ARM_ARCH >= 8)
#define VECTOR_SIZE_BYTES   16
#else
#define VECTOR_SIZE_BYTES   4
#endif

#define INT8_VECTOR_SIZE    (VECTOR_SIZE_BYTES / 1U)
#define UINT8_VECTOR_SIZE   (VECTOR_SIZE_BYTES / 1U)

#define INT16_VECTOR_SIZE   (VECTOR_SIZE_BYTES / 2U)
#define UINT16_VECTOR_SIZE  (VECTOR_SIZE_BYTES / 2U)

#define INT32_VECTOR_SIZE   (VECTOR_SIZE_BYTES / 4U)
#define UINT32_VECTOR_SIZE  (VECTOR_SIZE_BYTES / 4U)

#if (VECTOR_SIZE_BYTES >= 8)
#define INT64_VECTOR_SIZE   (VECTOR_SIZE_BYTES / 8U)
#define UINT64_VECTOR_SIZE  (VECTOR_SIZE_BYTES / 8U)
#endif

#if (__ARM_ARCH >= 8)
typedef int8x16_t  v128_s8_t;
typedef uint8x16_t v128_u8_t;

typedef int16x8_t  v128_s16_t;
typedef uint16x8_t v128_u16_t;

typedef int32x4_t  v128_s32_t;
typedef uint32x4_t v128_u32_t;

#if (VECTOR_SIZE_BYTES >= 8)
typedef int64x2_t  v128_s64_t;
typedef uint64x2_t v128_u64_t;
#endif

typedef mve_pred16_t v128_predicate_t;
#else
typedef int8_t  v128_s8_t __attribute__ ((vector_size(VECTOR_SIZE_BYTES)));
typedef uint8_t v128_u8_t __attribute__ ((vector_size(VECTOR_SIZE_BYTES)));

typedef int16_t  v128_s16_t __attribute__ ((vector_size(VECTOR_SIZE_BYTES)));
typedef uint16_t v128_u16_t __attribute__ ((vector_size(VECTOR_SIZE_BYTES)));

typedef int32_t  v128_s32_t __attribute__ ((vector_size(VECTOR_SIZE_BYTES)));
typedef uint32_t v128_u32_t __attribute__ ((vector_size(VECTOR_SIZE_BYTES)));

#if (VECTOR_SIZE_BYTES >= 8)
typedef int64_t  v128_s64_t __attribute__ ((vector_size(VECTOR_SIZE_BYTES)));
typedef uint64_t v128_u64_t __attribute__ ((vector_size(VECTOR_SIZE_BYTES)));
#endif

typedef uint32_t v128_predicate_t;
#endif

typedef union {
    v128_s8_t s8;
    v128_u8_t u8;
    v128_s16_t s16;
    v128_u16_t u16;
    v128_s32_t s32;
    v128_u32_t u32;
    #if (VECTOR_SIZE_BYTES >= 8)
    v128_s64_t s64;
    v128_u64_t u64;
    #endif
} v128_t;

// These structures are meant to be returned via inline functions so that the compiler can optimize
// them across function boundaries. DO NOT return these via reference as the compiler will NOT
// treat them as local variables anymore and will NOT optimize them across function boundaries.
// Note: Values in the structures are named on-purpose force constant indexing as they are meant to
// be optimized away by the compiler.

typedef union vrow_ptr {
    uint8_t *u8;
    int8_t *s8;
    uint16_t *u16;
    int16_t *s16;
    uint32_t *u32;
    int32_t *s32;
    #if (VECTOR_SIZE_BYTES >= 8)
    uint64_t *u64;
    int64_t *s64;
    #endif
} vrow_ptr_t;

typedef struct v2x_row_ptrs {
    vrow_ptr_t p0, p1;
} v2x_row_ptrs_t;

typedef struct v2x_rows {
    v128_t r0, r1;
} v2x_rows_t;

typedef struct v3x_row_ptrs {
    vrow_ptr_t p0, p1, p2;
} v3x_row_ptrs_t;

typedef struct v3x_rows {
    v128_t r0, r1, r2;
} v3x_rows_t;

typedef struct v4x_row_ptrs {
    vrow_ptr_t p0, p1, p2, p3;
} v4x_row_ptrs_t;

typedef struct v4x_rows {
    v128_t r0, r1, r2, r3;
} v4x_rows_t;

typedef struct vrgb_pixels {
    v128_t r, g, b;
} vrgb_pixels_t;

static inline v128_predicate_t vpredicate_8(uint32_t n) {
    #if (__ARM_ARCH >= 8)
    return vctp8q(n);
    #else
    return IM_MIN(n, UINT8_VECTOR_SIZE);
    #endif
}

static inline v128_predicate_t vpredicate_16(uint32_t n) {
    #if (__ARM_ARCH >= 8)
    return vctp16q(n);
    #else
    return IM_MIN(n, UINT16_VECTOR_SIZE);
    #endif
}

static inline v128_predicate_t vpredicate_32(uint32_t n) {
    #if (__ARM_ARCH >= 8)
    return vctp32q(n);
    #else
    return IM_MIN(n, UINT32_VECTOR_SIZE);
    #endif
}

#if (VECTOR_SIZE_BYTES >= 8)
static inline v128_predicate_t vpredicate_64(uint32_t n) {
    #if (__ARM_ARCH >= 8)
    return vctp64q(n);
    #else
    return IM_MIN(n, UINT64_VECTOR_SIZE);
    #endif
}
#endif

static inline uint32_t vpredicate_8_get_mask(v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return pred;
    #else
    return (1 << pred) - 1;
    #endif
}

static inline uint32_t vpredicate_16_get_mask(v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return pred;
    #else
    return (1 << (pred * 2)) - 1;
    #endif
}

static inline uint32_t vpredicate_32_get_mask(v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return pred;
    #else
    return (1 << (pred * 4)) - 1;
    #endif
}

#if (VECTOR_SIZE_BYTES >= 8)
static inline uint32_t vpredicate_64_get_mask(v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return pred;
    #else
    return (1 << (pred * 8)) - 1;
    #endif
}
#endif

static inline uint32_t vpredicate_8_get_n(v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return 32 - __CLZ(pred);
    #else
    return pred;
    #endif
}

static inline uint32_t vpredicate_16_get_n(v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return (32 - __CLZ(pred)) / 2;
    #else
    return pred;
    #endif
}

static inline uint32_t vpredicate_32_get_n(v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return (32 - __CLZ(pred)) / 4;
    #else
    return pred;
    #endif
}

static inline uint32_t vpredicate_64_get_n(v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return (32 - __CLZ(pred)) / 8;
    #else
    return pred;
    #endif
}

static inline bool vpredicate_8_all_lanes_active(v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return pred == ((1 << VECTOR_SIZE_BYTES) - 1);
    #else
    return pred == (VECTOR_SIZE_BYTES / 1U);
    #endif
}

static inline bool vpredicate_16_all_lanes_active(v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return pred == ((1 << VECTOR_SIZE_BYTES) - 1);
    #else
    return pred == (VECTOR_SIZE_BYTES / 2U);
    #endif
}

static inline bool vpredicate_32_all_lanes_active(v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return pred == ((1 << VECTOR_SIZE_BYTES) - 1);
    #else
    return pred == (VECTOR_SIZE_BYTES / 4U);
    #endif
}

static inline bool vpredicate_64_all_lanes_active(v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return pred == ((1 << VECTOR_SIZE_BYTES) - 1);
    #else
    return pred == (VECTOR_SIZE_BYTES / 8U);
    #endif
}

static inline v128_predicate_t vpredicate_8_add(v128_predicate_t pred, uint32_t x) {
    #if (__ARM_ARCH >= 8)
    return (pred << x) | ((1 << x) - 1);
    #else
    return pred + x;
    #endif
}

static inline v128_predicate_t vpredicate_16_add(v128_predicate_t pred, uint32_t x) {
    #if (__ARM_ARCH >= 8)
    return (pred << (x * 2)) | ((1 << (x * 2)) - 1);
    #else
    return pred + x;
    #endif
}

static inline v128_predicate_t vpredicate_32_add(v128_predicate_t pred, uint32_t x) {
    #if (__ARM_ARCH >= 8)
    return (pred << (x * 4)) | ((1 << (x * 4)) - 1);
    #else
    return pred + x;
    #endif
}

static inline v128_predicate_t vpredicate_64_add(v128_predicate_t pred, uint32_t x) {
    #if (__ARM_ARCH >= 8)
    return (pred << (x * 8)) | ((1 << (x * 8)) - 1);
    #else
    return pred + x;
    #endif
}

static inline v128_t vhadd_u8(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vhaddq(v0.u8, v1.u8);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .u32 = { __UHADD8(v0.u32[0], v1.u32[0]) }
    };
    #else
    return (v128_t) {
        .u8 = (v0.u8 + v1.u8) >> 1
    };
    #endif
}

static inline v128_t vhadd_s8(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vhaddq(v0.s8, v1.s8);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .s32 = { __SHADD8(v0.s32[0], v1.s32[0]) }
    };
    #else
    return (v128_t) {
        .s8 = (v0.s8 + v1.s8) >> 1
    };
    #endif
}

static inline v128_t vhadd_u16(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vhaddq(v0.u16, v1.u16);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .u32 = { __UHADD16(v0.u32[0], v1.u32[0]) }
    };
    #else
    return (v128_t) {
        .u16 = (v0.u16 + v1.u16) >> 1
    };
    #endif
}

static inline v128_t vhadd_s16(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vhaddq(v0.s16, v1.s16);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .s32 = { __SHADD16(v0.s32[0], v1.s32[0]) }
    };
    #else
    return (v128_t) {
        .s16 = (v0.s16 + v1.s16) >> 1
    };
    #endif
}

static inline v128_t vuxtb16(v128_t v0) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmovlbq(v0.u8);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .u32 = { __UXTB16(v0.u32[0]) }
    };
    #else
    v128_t r;
    r.u16[0] = v0.u8[0];
    r.u16[1] = v0.u8[2];
    return r;
    #endif
}

static inline v128_t vsxtb16(v128_t v0) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmovlbq(v0.s8);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .u32 = { __SXTB16(v0.u32[0]) }
    };
    #else
    v128_t r;
    r.s16[0] = v0.s8[0];
    r.s16[1] = v0.s8[2];
    return r;
    #endif
}

static inline v128_t vuxtb16_ror8(v128_t v0) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmovltq(v0.u8);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .u32 = { __UXTB16_RORn(v0.u32[0], 8) }
    };
    #else
    v128_t r;
    r.u16[0] = v0.u8[1];
    r.u16[1] = v0.u8[3];
    return r;
    #endif
}

static inline v128_t vsxtb16_ror8(v128_t v0) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmovltq(v0.s8);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .u32 = { __UXTB16_RORn(v0.u32[0], 8) }
    };
    #else
    v128_t r;
    r.s16[0] = v0.s8[1];
    r.s16[1] = v0.s8[3];
    return r;
    #endif
}

static inline v128_t vuxtb32(v128_t v0) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmovlbq(v0.u16);
    // #elif (__ARM_ARCH >= 7)
    // return (v128_t) {
    //     .u32 = { __UXTH(v0.u32[0]) }
    // };
    #else
    v128_t r;
    r.u32[0] = v0.u16[0];
    return r;
    #endif
}

static inline v128_t vsxtb32(v128_t v0) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmovlbq(v0.s16);
    // #elif (__ARM_ARCH >= 7)
    // return (v128_t) {
    //     .u32 = { __SXTH(v0.u32[0]) }
    // };
    #else
    v128_t r;
    r.s32[0] = v0.s16[0];
    return r;
    #endif
}

static inline v128_t vpkhbt(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vsliq_n_u32(v0.u32, v1.u32, 16);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .u32 = { __PKHBT(v0.u32[0], v1.u32[0], 16) }
    };
    #else
    v128_t r;
    r.u16[0] = v0.u16[0];
    r.u16[1] = v1.u16[0];
    return r;
    #endif
}

static inline v128_t vpkhbt_ror8(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vshrntq_n_u32(v0.u16, v1.u32, 8);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .u32 = { __PKHBT(v0.u32[0], v1.u32[0], 8) }
    };
    #else
    v128_t r;
    r.u16[0] = v0.u16[0];
    r.u16[1] = v1.u32[0] >> 8;
    return r;
    #endif
}

static inline v128_t vpkhtb(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vsriq_n_u32(v0.u32, v1.u32, 16);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .u32 = { __PKHTB(v0.u32[0], v1.u32[0], 16) }
    };
    #else
    v128_t r;
    r.u16[0] = v1.u16[1];
    r.u16[1] = v0.u16[1];
    return r;
    #endif
}

static inline v128_t vpkhtb_ror8(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vshrnbq_n_s32(v0.s16, v1.s32, 8);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .u32 = { __PKHTB(v0.u32[0], v1.u32[0], 8) }
    };
    #else
    v128_t r;
    r.s16[0] = v1.s32[0] >> 8;
    r.s16[1] = v0.s16[1];
    return r;
    #endif
}

static inline v128_t vmov_u16_narrow_u8_lo(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmovnbq(v0.u8, v1.u16);
    #elif (__ARM_ARCH >= 7)
    uint32_t t0 = v1.u32[0];
    uint32_t t1 = __USUB8(0xFF00FF00, 0x00FF00FF); (void) t1;
    return (v128_t) {
        .u32 = { __SEL(v0.u32[0], t0) }
    };
    #else
    return (v128_t) {
        .u32 = { (v0.u32[0] & 0xFF00FF00) | (v1.u32[0] & 0x00FF00FF) }
    };
    #endif
}

static inline v128_t vmov_u16_narrow_u8_hi(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmovntq(v0.u8, v1.u16);
    #elif (__ARM_ARCH >= 7)
    uint32_t t0 = v1.u32[0] << 8;
    uint32_t t1 = __USUB8(0x00FF00FF, 0xFF00FF00); (void) t1;
    return (v128_t) {
        .u32 = { __SEL(v0.u32[0], t0) }
    };
    #else
    return (v128_t) {
        .u32 = { (v0.u32[0] & 0x00FF00FF) | ((v1.u32[0] << 8) & 0xFF00FF00) }
    };
    #endif
}

#if (__ARM_ARCH >= 8)
#define vusat_s16_narrow_u8_lo(v0, v1, shift) ((v128_t) vqshrunbq_n_s16(v0.u8, v1.s16, shift))
#else
static inline v128_t vusat_s16_narrow_u8_lo(v128_t v0, v128_t v1, uint32_t shift) {
    #if (__ARM_ARCH >= 7)
    uint32_t t0 = __USAT16(v1.u32[0], 8 + shift) >> shift;
    uint32_t t1 = __USUB8(0xFF00FF00, 0x00FF00FF); (void) t1;
    return (v128_t) {
        .u32 = { __SEL(v0.u32[0], t0) }
    };
    #else // There's a software implementation of __USAT16 in the ARM CMSIS extension if needed
    return (v128_t) {
        .u32 = { (v0.u32[0] & 0xFF00FF00) | ((__USAT16(v1.u32[0], 8 + shift) >> shift) & 0x00FF00FF) }
    };
    #endif
}
#endif

#if (__ARM_ARCH >= 8)
#define vusat_s16_narrow_u8_hi(v0, v1, shift) ((v128_t) vqshruntq_n_s16(v0.u8, v1.s16, shift))
#else
static inline v128_t vusat_s16_narrow_u8_hi(v128_t v0, v128_t v1, uint32_t shift) {
    #if (__ARM_ARCH >= 7)
    uint32_t t0 = __USAT16(v1.u32[0], 8 + shift) << (8 - shift);
    uint32_t t1 = __USUB8(0x00FF00FF, 0xFF00FF00); (void) t1;
    return (v128_t) {
        .u32 = { __SEL(v0.u32[0], t0) }
    };
    #else // There's a software implementation of __USAT16 in the ARM CMSIS extension if needed
    return (v128_t) {
        .u32 = { (v0.u32[0] & 0x00FF00FF) | ((__USAT16(v1.u32[0], 8 + shift) << (8 - shift)) & 0xFF00FF00) }
    };
    #endif
}
#endif

#if (__ARM_ARCH >= 8)
#define vget_u8(v0, n) vgetq_lane_u8(v0.u8, n)
#else
static inline uint8_t vget_u8(v128_t v0, uint32_t n) {
    return v0.u8[n];
}
#endif

#if (__ARM_ARCH >= 8)
#define vget_s8(v0, n) vgetq_lane_s8(v0.s8, n)
#else
static inline int8_t vget_s8(v128_t v0, uint32_t n) {
    return v0.s8[n];
}
#endif

#if (__ARM_ARCH >= 8)
#define vget_u16(v0, n) vgetq_lane_u16(v0.u16, n)
#else
static inline uint16_t vget_u16(v128_t v0, uint32_t n) {
    return v0.u16[n];
}
#endif

#if (__ARM_ARCH >= 8)
#define vget_s16(v0, n) vgetq_lane_s16(v0.s16, n)
#else
static inline int16_t vget_s16(v128_t v0, uint32_t n) {
    return v0.s16[n];
}
#endif

#if (__ARM_ARCH >= 8)
#define vget_u32(v0, n) vgetq_lane_u32(v0.u32, n)
#else
static inline uint32_t vget_u32(v128_t v0, uint32_t n) {
    return v0.u32[n];
}
#endif

#if (__ARM_ARCH >= 8)
#define vget_s32(v0, n) vgetq_lane_s32(v0.s32, n)
#else
static inline int32_t vget_s32(v128_t v0, uint32_t n) {
    return v0.s32[n];
}
#endif

#if (VECTOR_SIZE_BYTES >= 8)
#if (__ARM_ARCH >= 8)
#define vget_u64(v0, n) vgetq_lane_u64(v0.u64, n)
#else
static inline uint64_t vget_u64(v128_t v0, uint32_t n) {
    return v0.u64[n];
}
#endif

#if (__ARM_ARCH >= 8)
#define vget_s64(v0, n) vgetq_lane_s64(v0.s64, n)
#else
static inline int64_t vget_s64(v128_t v0, uint32_t n) {
    return v0.s64[n];
}
#endif
#endif

#if (__ARM_ARCH >= 8)
#define vset_u8(v0, n, x) ((v128_t) vsetq_lane_u8(x, v0.u8, n))
#else
static inline v128_t vset_u8(v128_t v0, uint32_t n, uint8_t x) {
    v0.u8[n] = x;
    return v0;
}
#endif

#if (__ARM_ARCH >= 8)
#define vset_s8(v0, n, x) ((v128_t) vsetq_lane_s8(x, v0.s8, n))
#else
static inline v128_t vset_s8(v128_t v0, uint32_t n, int8_t x) {
    v0.s8[n] = x;
    return v0;
}
#endif

#if (__ARM_ARCH >= 8)
#define vset_u16(v0, n, x) ((v128_t) vsetq_lane_u16(x, v0.u16, n))
#else
static inline v128_t vset_u16(v128_t v0, uint32_t n, uint16_t x) {
    v0.u16[n] = x;
    return v0;
}
#endif

#if (__ARM_ARCH >= 8)
#define vset_s16(v0, n, x) ((v128_t) vsetq_lane_s16(x, v0.s16, n))
#else
static inline v128_t vset_s16(v128_t v0, uint32_t n, int16_t x) {
    v0.s16[n] = x;
    return v0;
}
#endif

#if (__ARM_ARCH >= 8)
#define vset_u32(v0, n, x) ((v128_t) vsetq_lane_u32(x, v0.u32, n))
#else
static inline v128_t vset_u32(v128_t v0, uint32_t n, uint32_t x) {
    v0.u32[n] = x;
    return v0;
}
#endif

#if (__ARM_ARCH >= 8)
#define vset_s32(v0, n, x) ((v128_t) vsetq_lane_s32(x, v0.s32, n))
#else
static inline v128_t vset_s32(v128_t v0, uint32_t n, int32_t x) {
    v0.s32[n] = x;
    return v0;
}
#endif

#if (VECTOR_SIZE_BYTES >= 8)
#if (__ARM_ARCH >= 8)
#define vset_u64(v0, n, x) ((v128_t) vsetq_lane_u64(x, v0.u64, n))
#else
static inline v128_t vset_u64(v128_t v0, uint32_t n, uint64_t x) {
    v0.u64[n] = x;
    return v0;
}
#endif

#if (__ARM_ARCH >= 8)
#define vset_s64(v0, n, x) ((v128_t) vsetq_lane_s64(x, v0.s64, n))
#else
static inline v128_t vset_s64(v128_t v0, uint32_t n, int64_t x) {
    v0.s64[n] = x;
    return v0;
}
#endif
#endif

// GCC does not vectorize assignment from a scalar to a vector.
#if (__ARM_ARCH >= 8)
#define vdup_u8(x) ((v128_t) vdupq_n_u8(x))
#else
static inline v128_t vdup_u8(uint8_t x) {
    return (v128_t) {
        .u32 = { x * 0x01010101 }
    };
}
#endif

// GCC does not vectorize assignment from a scalar to a vector.
#if (__ARM_ARCH >= 8)
#define vdup_s8(x) ((v128_t) vdupq_n_s8(x))
#else
static inline v128_t vdup_s8(int8_t x) {
    return (v128_t) {
        .s32 = { (x & 0xFF) * 0x01010101 }
    };
}
#endif

// GCC does not vectorize assignment from a scalar to a vector.
#if (__ARM_ARCH >= 8)
#define vdup_u16(x) ((v128_t) vdupq_n_u16(x))
#else
static inline v128_t vdup_u16(uint16_t x) {
    return (v128_t) {
        .u32 = { x * 0x00010001 }
    };
}
#endif

// GCC does not vectorize assignment from a scalar to a vector.
#if (__ARM_ARCH >= 8)
#define vdup_s16(x) ((v128_t) vdupq_n_s16(x))
#else
static inline v128_t vdup_s16(int16_t x) {
    return (v128_t) {
        .s32 = { (x & 0xFFFF) * 0x00010001 }
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vdup_u32(x) ((v128_t) vdupq_n_u32(x))
#else
static inline v128_t vdup_u32(uint32_t x) {
    return (v128_t) {
        .u32 = { x }
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vdup_s32(x) ((v128_t) vdupq_n_s32(x))
#else
static inline v128_t vdup_s32(int32_t x) {
    return (v128_t) {
        .s32 = { x }
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vidup_u8(start, increment) ((v128_t) vidupq_n_u8(start, increment))
#else
static inline v128_t vidup_u8(uint32_t start, uint32_t increment) {
    v128_t r;
    r.u8[0] = start;
    r.u8[1] = start + increment;
    r.u8[2] = start + (increment * 2);
    r.u8[3] = start + (increment * 3);
    return r;
}
#endif

#if (__ARM_ARCH >= 8)
#define vidup_u16(start, increment) ((v128_t) vidupq_n_u16(start, increment))
#else
static inline v128_t vidup_u16(uint32_t start, uint32_t increment) {
    v128_t r;
    r.u16[0] = start;
    r.u16[1] = start + increment;
    return r;
}
#endif

#if (__ARM_ARCH >= 8)
#define vidup_u32(start, increment) ((v128_t) vidupq_n_u32(start, increment))
#else
static inline v128_t vidup_u32(uint32_t start, uint32_t increment) {
    return (v128_t) {
        .u32 = { start }
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vidup_u32_unaligned(start, increment) ({                  \
        v128_t offsets = (v128_t) vidupq_n_u32(start, increment); \
        offsets.u32 = vsliq_n_u32(offsets.u32, offsets.u32, 8);   \
        offsets.u32 = vsliq_n_u32(offsets.u32, offsets.u32, 16);  \
        offsets.u8 = vaddq(offsets.u8, viwdupq_n_u8(0, 4, 1));    \
        offsets;                                                  \
    })
#else
static inline v128_t vidup_u32_unaligned(uint32_t start, uint32_t increment) {
    return (v128_t) {
        .u32 = { start }
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vshlc(v0, reg, n) ((v128_t) vshlcq(v0.u32, reg, n))
#else
static inline v128_t vshlc(v128_t v0, uint32_t *reg, uint32_t n) {
    v128_t r = (v128_t) {
        .u32 = { (v0.u32[0] << n) | ((*reg) & ((1 << n) - 1)) }
    };
    *reg = v0.u32[0] >> (32 - n);
    return r;
}
#endif

static inline v128_t vadd_u32(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vaddq(v0.u32, v1.u32);
    #else
    return (v128_t) {
        .u32 = v0.u32 + v1.u32
    };
    #endif
}

static inline v128_t vadd_s32(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vaddq(v0.s32, v1.s32);
    #else
    return (v128_t) {
        .s32 = v0.s32 + v1.s32
    };
    #endif
}

static inline v128_t vsub_u8(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vsubq(v0.u8, v1.u8);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .u32 = { __USUB8(v0.u32[0], v1.u32[0]) }
    };
    #else
    return (v128_t) {
        .u8 = v0.u8 - v1.u8
    };
    #endif
}

static inline v128_t vsub_s8(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vsubq(v0.s8, v1.s8);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .u32 = { __SSUB8(v0.u32[0], v1.u32[0]) }
    };
    #else
    return (v128_t) {
        .s8 = v0.s8 - v1.s8
    };
    #endif
}

static inline v128_t vsub_u16(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vsubq(v0.u16, v1.u16);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .u32 = { __USUB16(v0.u32[0], v1.u32[0]) }
    };
    #else
    return (v128_t) {
        .u16 = v0.u16 - v1.u16
    };
    #endif
}

static inline v128_t vsub_s16(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vsubq(v0.s16, v1.s16);
    #elif (__ARM_ARCH >= 7)
    return (v128_t) {
        .u32 = { __SSUB16(v0.u32[0], v1.u32[0]) }
    };
    #else
    return (v128_t) {
        .s16 = v0.s16 - v1.s16
    };
    #endif
}

#if (__ARM_ARCH >= 8)
#define vsli_u8(v0, v1, n) ((v128_t) vsliq_n_u8(v0.u8, v1.u8, n))
#else
static inline v128_t vsli_u8(v128_t v0, v128_t v1, uint32_t n) {
    uint8_t mask = (1 << n) - 1;
    return (v128_t) {
        .u8 = (v1.u8 << n) | (v0.u8 & mask)
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vsli_u16(v0, v1, n) ((v128_t) vsliq_n_u16(v0.u16, v1.u16, n))
#else
static inline v128_t vsli_u16(v128_t v0, v128_t v1, uint32_t n) {
    uint16_t mask = (1 << n) - 1;
    return (v128_t) {
        .u16 = (v1.u16 << n) | (v0.u16 & mask)
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vsli_u32(v0, v1, n) ((v128_t) vsliq_n_u32(v0.u32, v1.u32, n))
#else
static inline v128_t vsli_u32(v128_t v0, v128_t v1, uint32_t n) {
    uint32_t mask = (1 << n) - 1;
    return (v128_t) {
        .u32 = (v1.u32 << n) | (v0.u32 & mask)
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vsri_u8(v0, v1, n) ((v128_t) vsriq_n_u8(v0.u8, v1.u8, n))
#else
static inline v128_t vsri_u8(v128_t v0, v128_t v1, uint32_t n) {
    uint8_t mask = ~((1 << (8 - n)) - 1);
    return (v128_t) {
        .u8 = (v1.u8 >> n) | (v0.u8 & mask)
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vsri_u16(v0, v1, n) ((v128_t) vsriq_n_u16(v0.u16, v1.u16, n))
#else
static inline v128_t vsri_u16(v128_t v0, v128_t v1, uint32_t n) {
    uint16_t mask = ~((1 << (16 - n)) - 1);
    return (v128_t) {
        .u16 = (v1.u16 >> n) | (v0.u16 & mask)
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vsri_u32(v0, v1, n) ((v128_t) vsriq_n_u32(v0.u32, v1.u32, n))
#else
static inline v128_t vsri_u32(v128_t v0, v128_t v1, uint32_t n) {
    uint32_t mask = ~((1 << (32 - n)) - 1);
    return (v128_t) {
        .u32 = (v1.u32 >> n) | (v0.u32 & mask)
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vasr_s16(v0, n) ((v128_t) vshrq(v0.s16, n))
#else
static inline v128_t vasr_s16(v128_t v0, uint32_t n) {
    return (v128_t) {
        .s16 = v0.s16 >> n
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vasr_s32(v0, n) ((v128_t) vshrq(v0.s32, n))
#else
static inline v128_t vasr_s32(v128_t v0, uint32_t n) {
    return (v128_t) {
        .s32 = v0.s32 >> n
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vlsl_u16(v0, n) ((v128_t) vshlq_n(v0.u16, n))
#else
static inline v128_t vlsl_u16(v128_t v0, uint32_t n) {
    return (v128_t) {
        .u16 = v0.u16 << n
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vlsl_u32(v0, n) ((v128_t) vshlq_n(v0.u32, n))
#else
static inline v128_t vlsl_u32(v128_t v0, uint32_t n) {
    return (v128_t) {
        .u32 = v0.u32 << n
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vlsl_s16(v0, n) ((v128_t) vshlq_n(v0.s16, n))
#else
static inline v128_t vlsl_s16(v128_t v0, uint32_t n) {
    return (v128_t) {
        .s16 = v0.s16 << n
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vlsl_s32(v0, n) ((v128_t) vshlq_n(v0.s32, n))
#else
static inline v128_t vlsl_s32(v128_t v0, uint32_t n) {
    return (v128_t) {
        .s32 = v0.s32 << n
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vlsr_u16(v0, n) ((v128_t) vshrq(v0.u16, n))
#else
static inline v128_t vlsr_u16(v128_t v0, uint32_t n) {
    return (v128_t) {
        .u16 = v0.u16 >> n
    };
}
#endif

#if (__ARM_ARCH >= 8)
#define vlsr_u32(v0, n) ((v128_t) vshrq(v0.u32, n))
#else
static inline v128_t vlsr_u32(v128_t v0, uint32_t n) {
    return (v128_t) {
        .u32 = v0.u32 >> n
    };
}
#endif

static inline v128_t vand_u32(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vandq(v0.u32, v1.u32);
    #else
    return (v128_t) {
        .u32 = v0.u32 & v1.u32
    };
    #endif
}

static inline v128_t vshl_u8(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vshlq(v0.u8, v1.s8);
    #else
    return (v128_t) {
        .u8 = v0.u8 << v1.u8
    };
    #endif
}

static inline v128_t vshl_u16(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vshlq(v0.u16, v1.s16);
    #else
    return (v128_t) {
        .u16 = v0.u16 << v1.u16
    };
    #endif
}

static inline v128_t vand_s32(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vandq(v0.s32, v1.s32);
    #else
    return (v128_t) {
        .s32 = v0.s32 & v1.s32
    };
    #endif
}

static inline v128_t vorr_u32(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vorrq(v0.u32, v1.u32);
    #else
    return (v128_t) {
        .u32 = v0.u32 | v1.u32
    };
    #endif
}

static inline v128_t vorr_s32(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vorrq(v0.s32, v1.s32);
    #else
    return (v128_t) {
        .s32 = v0.s32 | v1.s32
    };
    #endif
}

static inline v128_t veor_u32(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) veorq(v0.u32, v1.u32);
    #else
    return (v128_t) {
        .u32 = v0.u32 ^ v1.u32
    };
    #endif
}

static inline v128_t veor_s32(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) veorq(v0.s32, v1.s32);
    #else
    return (v128_t) {
        .s32 = v0.s32 ^ v1.s32
    };
    #endif
}

static inline v128_t vmul_u32(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmulq(v0.u32, v1.u32);
    #else
    return (v128_t) {
        .u32 = v0.u32 * v1.u32
    };
    #endif
}

static inline v128_t vmul_s32(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmulq(v0.s32, v1.s32);
    #else
    return (v128_t) {
        .s32 = v0.s32 * v1.s32
    };
    #endif
}

static inline v128_t vmla_u32(v128_t v0, v128_t v1, v128_t v2) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vaddq(vmulq(v0.u32, v1.u32), v2.u32);
    #else
    return (v128_t) {
        .u32 = (v0.u32 * v1.u32) + v2.u32
    };
    #endif
}

static inline v128_t vmla_s32(v128_t v0, v128_t v1, v128_t v2) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vaddq(vmulq(v0.s32, v1.s32), v2.s32);
    #else
    return (v128_t) {
        .s32 = (v0.s32 * v1.s32) + v2.s32
    };
    #endif
}

static inline v128_t vmul_n_u16(v128_t v0, uint16_t x) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmulq_n_u16(v0.u16, x);
    #else
    return (v128_t) {
        .u16 = v0.u16 * x
    };
    #endif
}

static inline v128_t vmul_n_u32(v128_t v0, uint32_t x) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmulq_n_u32(v0.u32, x);
    #else
    return (v128_t) {
        .u32 = v0.u32 * x
    };
    #endif
}

static inline v128_t vmul_n_s16(v128_t v0, int16_t x) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmulq_n_s16(v0.s16, x);
    #else
    return (v128_t) {
        .s16 = v0.s16 * x
    };
    #endif
}

static inline v128_t vmul_n_s32(v128_t v0, int32_t x) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmulq_n_s32(v0.s32, x);
    #else
    return (v128_t) {
        .s32 = v0.s32 * x
    };
    #endif
}

static inline v128_t vmla_n_u16(v128_t v0, uint16_t x, v128_t v2) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmlaq_n_u16(v2.u16, v0.u16, x);
    #else
    return (v128_t) {
        .u16 = (v0.u16 * x) + v2.u16
    };
    #endif
}

static inline v128_t vmla_n_u32(v128_t v0, uint32_t x, v128_t v2) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmlaq_n_u32(v2.u32, v0.u32, x);
    #else
    return (v128_t) {
        .u32 = (v0.u32 * x) + v2.u32
    };
    #endif
}

static inline v128_t vmla_n_s16(v128_t v0, int16_t x, v128_t v2) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmlaq_n_s16(v2.s16, v0.s16, x);
    #else
    return (v128_t) {
        .s16 = (v0.s16 * x) + v2.s16
    };
    #endif
}

static inline v128_t vmla_n_s32(v128_t v0, int32_t x, v128_t v2) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vmlaq_n_s32(v2.s32, v0.s32, x);
    #else
    return (v128_t) {
        .s32 = (v0.s32 * x) + v2.s32
    };
    #endif
}

static inline uint32_t vmladav_u16(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return vmladavq_u16(v0.u16, v1.u16);
    #else
    return (v0.u16[0] * v1.u16[0]) + (v0.u16[1] * v1.u16[1]);
    #endif
}

static inline int32_t vmladav_s16(v128_t v0, v128_t v1) {
    #if (__ARM_ARCH >= 8)
    return vmladavq_s16(v0.s16, v1.s16);
    #elif (__ARM_ARCH >= 7)
    return __SMUAD(v0.u32[0], v1.u32[0]);
    #else
    return (v0.s16[0] * v1.s16[0]) + (v0.s16[1] * v1.s16[1]);
    #endif
}

static inline uint32_t vmladava_u16(v128_t v0, v128_t v1, uint32_t acc) {
    #if (__ARM_ARCH >= 8)
    return vmladavaq_u16(acc, v0.u16, v1.u16);
    #else
    return acc + (v0.u16[0] * v1.u16[0]) + (v0.u16[1] * v1.u16[1]);
    #endif
}

static inline int32_t vmladava_s16(v128_t v0, v128_t v1, int32_t acc) {
    #if (__ARM_ARCH >= 8)
    return vmladavaq_s16(acc, v0.s16, v1.s16);
    #elif (__ARM_ARCH >= 7)
    return __SMLAD(v0.u32[0], v1.u32[0], acc);
    #else
    return acc + (v0.s16[0] * v1.s16[0]) + (v0.s16[1] * v1.s16[1]);
    #endif
}

static inline v128_t vldr_u8(const uint8_t *p) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vldrbq_u8(p);
    #else
    return (v128_t) {
        .u32 = { *((const uint32_t *) p) }
    };
    #endif
}

static inline v128_t vldr_u8_pred(const uint8_t *p, v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vldrbq_z_u8(p, pred);
    #else
    v128_t v0;

    if (pred > 3) {
        v0.u32[0] = *((const uint32_t *) p);
    } else if (pred > 2) {
        v0.u32[0] = *((const uint16_t *) p);
        v0.u8[2] = p[2];
    } else if (pred > 1) {
        v0.u32[0] = *((const uint16_t *) p);
    } else {
        v0.u32[0] = p[0];
    }

    return v0;
    #endif
}

static inline void vstr_u8(uint8_t *p, v128_t v0) {
    #if (__ARM_ARCH >= 8)
    vstrbq(p, v0.u8);
    #else
    *((uint32_t *) p) = v0.u32[0];
    #endif
}

static inline void vstr_u8_pred(uint8_t *p, v128_t v0, v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    vstrbq_p_u8(p, v0.u8, pred);
    #else
    if (pred > 3) {
        *((uint32_t *) p) = v0.u32[0];
    } else if (pred > 2) {
        *((uint16_t *) p) = v0.u16[0];
        p[2] = v0.u8[2];
    } else if (pred > 1) {
        *((uint16_t *) p) = v0.u16[0];
    } else {
        p[0] = v0.u8[0];
    }
    #endif
}

static inline v128_t vldr_u16(const uint16_t *p) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vldrhq_u16(p);
    #else
    return (v128_t) {
        .u32 = { *((const uint32_t *) p) }
    };
    #endif
}

static inline v128_t vldr_u16_pred(const uint16_t *p, v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vldrhq_z_u16(p, pred);
    #else
    v128_t v0;

    if (pred > 1) {
        v0.u32[0] = *((const uint32_t *) p);
    } else {
        v0.u32[0] = p[0];
    }

    return v0;
    #endif
}

static inline void vstr_u16(uint16_t *p, v128_t v0) {
    #if (__ARM_ARCH >= 8)
    vstrhq(p, v0.u16);
    #else
    *((uint32_t *) p) = v0.u32[0];
    #endif
}

static inline void vstr_u16_pred(uint16_t *p, v128_t v0, v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    vstrhq_p_u16(p, v0.u16, pred);
    #else
    if (pred > 1) {
        *((uint32_t *) p) = v0.u32[0];
    } else {
        p[0] = v0.u16[0];
    }
    #endif
}

static inline v128_t vldr_u8_widen_u16_pred(uint8_t *p, v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    return (v128_t) vldrbq_z_u16(p, pred);
    #else
    v128_t v0;

    if (pred > 1) {
        v0.u32[0] = *((uint16_t *) p);
        v0.u8[2] = v0.u8[1];
        v0.u8[1] = 0;
    } else {
        v0.u32[0] = *p;
    }

    return v0;
    #endif
}

static inline void vstr_u16_narrow_u8_pred(uint8_t *p, v128_t v0, v128_predicate_t pred) {
    #if (__ARM_ARCH >= 8)
    vstrbq_p_u16(p, v0.u16, pred);
    #else
    *p = v0.u32[0];
    if (pred > 1) {
        *((uint8_t *) (p + 1)) = v0.u8[2];
    }
    #endif
}

static inline v128_t vldr_u32_gather_unaligned(const uint8_t *p, v128_t offsets) {
    #if (__ARM_ARCH >= 8)
    // vldrwq_gather_offset cannot handle unaligned loads.
    return (v128_t) vldrbq_gather_offset(p, offsets.u8);
    #else
    return (v128_t) {
        .u32 = { *((const uint32_t *) (p + offsets.u32[0])) }
    };
    #endif
}

static inline v4x_rows_t vldr_u32_gather_pred_x4_unaligned(v4x_row_ptrs_t rowptrs,
                                                           uint32_t x,
                                                           v128_t offsets,
                                                           v128_predicate_t pred) {
    const uint8_t *p0 = rowptrs.p0.u8 + x;
    const uint8_t *p1 = rowptrs.p1.u8 + x;
    const uint8_t *p2 = rowptrs.p2.u8 + x;
    const uint8_t *p3 = rowptrs.p3.u8 + x;
    v4x_rows_t rows;
    #if (__ARM_ARCH >= 8)
    // TODO: Move into a predicate block.
    // vldrwq_gather_offset_z_u32 cannot handle unaligned loads.
    rows.r0 = (v128_t) vldrbq_gather_offset_z_u8(p0, offsets.u8, pred);
    rows.r1 = (v128_t) vldrbq_gather_offset_z_u8(p1, offsets.u8, pred);
    rows.r2 = (v128_t) vldrbq_gather_offset_z_u8(p2, offsets.u8, pred);
    rows.r3 = (v128_t) vldrbq_gather_offset_z_u8(p3, offsets.u8, pred);
    #else
    if (pred > 3) {
        rows.r0.u32[0] = *((const uint32_t *) (p0 + offsets.u32[0]));
        rows.r1.u32[0] = *((const uint32_t *) (p1 + offsets.u32[0]));
        rows.r2.u32[0] = *((const uint32_t *) (p2 + offsets.u32[0]));
        rows.r3.u32[0] = *((const uint32_t *) (p3 + offsets.u32[0]));
    } else if (pred > 2) {
        rows.r0.u32[0] = *((const uint16_t *) (p0 + offsets.u32[0]));
        rows.r1.u32[0] = *((const uint16_t *) (p1 + offsets.u32[0]));
        rows.r2.u32[0] = *((const uint16_t *) (p2 + offsets.u32[0]));
        rows.r3.u32[0] = *((const uint16_t *) (p3 + offsets.u32[0]));
        rows.r0.u8[2] = p0[2 + offsets.u32[0]];
        rows.r1.u8[2] = p1[2 + offsets.u32[0]];
        rows.r2.u8[2] = p2[2 + offsets.u32[0]];
        rows.r3.u8[2] = p3[2 + offsets.u32[0]];
    } else if (pred > 1) {
        rows.r0.u32[0] = *((const uint16_t *) (p0 + offsets.u32[0]));
        rows.r1.u32[0] = *((const uint16_t *) (p1 + offsets.u32[0]));
        rows.r2.u32[0] = *((const uint16_t *) (p2 + offsets.u32[0]));
        rows.r3.u32[0] = *((const uint16_t *) (p3 + offsets.u32[0]));
    } else {
        rows.r0.u32[0] = p0[offsets.u32[0]];
        rows.r1.u32[0] = p1[offsets.u32[0]];
        rows.r2.u32[0] = p2[offsets.u32[0]];
        rows.r3.u32[0] = p3[offsets.u32[0]];
    }
    #endif
    return rows;
}

static inline v2x_rows_t vld2_u8(const uint8_t *p) {
    #if (__ARM_ARCH >= 8)
    uint8x16x2_t r = vld2q(p);
    v2x_rows_t rows;
    rows.r0.u8 = r.val[0];
    rows.r1.u8 = r.val[1];
    return rows;
    #else
    v128_t r0;
    r0.u8[0] = p[0];
    r0.u8[1] = p[2];
    r0.u8[2] = p[4];
    r0.u8[3] = p[6];
    v128_t r1;
    r1.u8[0] = p[1];
    r1.u8[1] = p[3];
    r1.u8[2] = p[5];
    r1.u8[3] = p[7];
    return (v2x_rows_t) {
        .r0 = r0,
        .r1 = r1
    };
    #endif
}

static inline v2x_rows_t vld2_u8_len(const uint8_t *p, uint32_t len) {
    len = (len > (UINT8_VECTOR_SIZE * 2)) ? (UINT8_VECTOR_SIZE * 2) : len;

    v2x_rows_t rows;

    for (uint32_t i = 0; i < len; i++) {
        if (i % 2) {
            rows.r1.u8[i / 2] = p[i];
        } else {
            rows.r0.u8[i / 2] = p[i];
        }
    }

    return rows;
}

static inline v2x_rows_t vld2_u16(const uint16_t *p) {
    #if (__ARM_ARCH >= 8)
    uint16x8x2_t r = vld2q(p);
    v2x_rows_t rows;
    rows.r0.u16 = r.val[0];
    rows.r1.u16 = r.val[1];
    return rows;
    #else
    v128_t r0;
    r0.u16[0] = p[0];
    r0.u16[1] = p[2];
    v128_t r1;
    r1.u16[0] = p[1];
    r1.u16[1] = p[3];
    return (v2x_rows_t) {
        .r0 = r0,
        .r1 = r1
    };
    #endif
}

static inline void vst2_u8(uint8_t *p, v2x_rows_t v0) {
    #if (__ARM_ARCH >= 8)
    uint8x16x2_t rows;
    rows.val[0] = v0.r0.u8;
    rows.val[1] = v0.r1.u8;
    vst2q(p, rows);
    #else
    p[0] = v0.r0.u8[0];
    p[1] = v0.r1.u8[0];
    p[2] = v0.r0.u8[1];
    p[3] = v0.r1.u8[1];
    p[4] = v0.r0.u8[2];
    p[5] = v0.r1.u8[2];
    p[6] = v0.r0.u8[3];
    p[7] = v0.r1.u8[3];
    #endif
}

static inline void vst2_u16(uint16_t *p, v2x_rows_t v0) {
    #if (__ARM_ARCH >= 8)
    uint16x8x2_t rows;
    rows.val[0] = v0.r0.u16;
    rows.val[1] = v0.r1.u16;
    vst2q(p, rows);
    #else
    p[0] = v0.r0.u16[0];
    p[1] = v0.r1.u16[0];
    p[2] = v0.r0.u16[1];
    p[3] = v0.r1.u16[1];
    #endif
}

static inline void vst2_u16_len(uint16_t *p, v2x_rows_t v0, uint32_t len) {
    len = (len > (UINT16_VECTOR_SIZE * 2)) ? (UINT16_VECTOR_SIZE * 2) : len;

    for (uint32_t i = 0; i < len; i++) {
        if (i % 2) {
            p[i] = v0.r1.u16[i / 2];
        } else {
            p[i] = v0.r0.u16[i / 2];
        }
    }
}

// n is in bytes, but, known to be a multiple of 1 with 1-byte alignment.
static inline void vmemcpy_8(void *dest, void *src, size_t n) {
    #if (__ARM_ARCH >= 8)
    uint8_t *dest8 = (uint8_t *) dest;
    uint8_t *src8 = (uint8_t *) src;
    for (; ((int32_t) n) > 0; n -= UINT8_VECTOR_SIZE) {
        mve_pred16_t p = vctp8q(n);
        vstrbq_p_u8(dest8, vldrbq_z_u8(src8, p), p);
        dest8 += UINT8_VECTOR_SIZE;
        src8 += UINT8_VECTOR_SIZE;
    }
    #elif (__ARM_ARCH > 6)
    // ARM Cortex-M4/M7 Processors can access memory using unaligned 32-bit reads/writes.
    uint32_t *dest32 = (uint32_t *) dest;
    uint32_t *src32 = (uint32_t *) src;

    for (; n > 4; n -= 4) {
        *dest32++ = *src32++;
    }

    uint8_t *dest8 = (uint8_t *) dest32;
    uint8_t *src8 = (uint8_t *) src32;

    for (; n > 0; n -= 1) {
        *dest8++ = *src8++;
    }
    #else
    memcpy(dest, src, n);
    #endif
}

// n is in bytes, but, known to be a multiple of 2 with 2-byte alignment.
static inline void vmemcpy_16(void *dest, void *src, size_t n) {
    #if (__ARM_ARCH >= 8)
    n = n / 2;
    uint16_t *dest16 = (uint16_t *) dest;
    uint16_t *src16 = (uint16_t *) src;
    for (; ((int32_t) n) > 0; n -= UINT16_VECTOR_SIZE) {
        mve_pred16_t p = vctp16q(n);
        vstrhq_p_u16(dest16, vldrhq_z_u16(src16, p), p);
        dest16 += UINT16_VECTOR_SIZE;
        src16 += UINT16_VECTOR_SIZE;
    }
    #elif (__ARM_ARCH > 6)
    // ARM Cortex-M4/M7 Processors can access memory using unaligned 32-bit reads/writes.
    uint32_t *dest32 = (uint32_t *) dest;
    uint32_t *src32 = (uint32_t *) src;

    for (; n > 4; n -= 4) {
        *dest32++ = *src32++;
    }

    uint16_t *dest16 = (uint16_t *) dest32;
    uint16_t *src16 = (uint16_t *) src32;

    for (; n > 0; n -= 2) {
        *dest16++ = *src16++;
    }
    #else
    memcpy(dest, src, n);
    #endif
}

// n is in bytes, but, known to be a multiple of 4 with 4-byte alignment.
static inline void vmemcpy_32(void *dest, void *src, size_t n) {
    #if (__ARM_ARCH >= 8)
    n = n / 4;
    uint32_t *dest32 = (uint32_t *) dest;
    uint32_t *src32 = (uint32_t *) src;
    for (; ((int32_t) n) > 0; n -= UINT32_VECTOR_SIZE) {
        mve_pred16_t p = vctp32q(n);
        vstrwq_p_u32(dest32, vldrwq_z_u32(src32, p), p);
        dest32 += UINT32_VECTOR_SIZE;
        src32 += UINT32_VECTOR_SIZE;
    }
    #elif (__ARM_ARCH > 6)
    uint32_t *dest32 = (uint32_t *) dest;
    uint32_t *src32 = (uint32_t *) src;

    for (; n > 0; n -= 4) {
        *dest32++ = *src32++;
    }
    #else
    memcpy(dest, src, n);
    #endif
}

#if (VECTOR_SIZE_BYTES >= 8)
// n is in bytes, but, known to be a multiple of 8 with 8-byte alignment.
static inline void vmemcpy_64(void *dest, void *src, size_t n) {
    // There are no 64-bit vector load and store instructions.
    #if (__ARM_ARCH > 6)
    uint64_t *dest64 = (uint64_t *) dest;
    uint64_t *src64 = (uint64_t *) src;

    for (; n > 0; n -= 8) {
        *dest64++ = *src64++;
    }
    #else
    memcpy(dest, src, n);
    #endif
}
#endif

// In the case of vectors larger than 32-bits the pattern is repeated for every 32-bits.
//
// pixels.r = MSB [0, R1, 0, R0] LSB pixels where each pixel is 8-bits.
// pixels.g = MSB [0, G1, 0, G0] LSB pixels where each pixel is 8-bits.
// pixels.b = MSB [0, B1, 0, B0] LSB pixels where each pixel is 8-bits.
//
// Y == ((R * 38) + (G * 75) + (B * 15)) / 128
//
// Returns 2x uint8_t Grayscale (MSB [garbage, G1, garbage, G0] LSB) pixels for every 32-bits.
static inline v128_t vrgb_pixels_to_grayscale(vrgb_pixels_t pixels) {
    pixels.r = vmul_n_u32(pixels.r, 38);
    pixels.r = vmla_n_u32(pixels.g, 75, pixels.r);
    pixels.r = vmla_n_u32(pixels.b, 15, pixels.r);
    return vlsr_u32(pixels.r, 7);
}

// In the case of vectors larger than 32-bits the pattern is repeated for every 32-bits.
//
// pixels.r = MSB [0, R1, 0, R0] LSB pixels where each pixel is 8-bits.
// pixels.g = MSB [0, G1, 0, G0] LSB pixels where each pixel is 8-bits.
// pixels.b = MSB [0, B1, 0, B0] LSB pixels where each pixel is 8-bits.
//
// Returns 2x uint16_t RGB565 (MSB [RGB1, RGB0] LSB) pixels for every 32-bits.
static inline v128_t vrgb_pixels_to_rgb565(vrgb_pixels_t pixels) {
    #if (__ARM_ARCH >= 8)
    pixels.r = vlsr_u16(pixels.r, 3);
    pixels.g = vlsr_u16(pixels.g, 2);
    pixels.b = vlsr_u16(pixels.b, 3);
    return vsli_u16(vsli_u16(pixels.b, pixels.g, 5), pixels.r, 11);
    #else
    pixels.r = vand_u32(vlsl_u32(pixels.r, 8), vdup_u16(0xf800));
    pixels.g = vand_u32(vlsl_u32(pixels.g, 3), vdup_u16(0x07e0));
    pixels.b = vand_u32(vlsr_u32(pixels.b, 3), vdup_u16(0x001f));
    return vorr_u32(pixels.r, vorr_u32(pixels.g, pixels.b));
    #endif
}

// In the case of vectors larger than 32-bits the pattern is repeated for every 32-bits.
//
// pixels.r = MSB [0, R1, 0, R0] LSB pixels where each pixel is 8-bits.
// pixels.g = MSB [0, G1, 0, G0] LSB pixels where each pixel is 8-bits.
// pixels.b = MSB [0, B1, 0, B0] LSB pixels where each pixel is 8-bits.
//
// Stores 2x GRAYSCALE pixels for every 32-bits.
static inline void vrgb_pixels_store_grayscale(uint8_t *p, uint32_t x, vrgb_pixels_t pixels, v128_predicate_t pred) {
    vstr_u16_narrow_u8_pred(p + x, vrgb_pixels_to_grayscale(pixels), pred);
}

// In the case of vectors larger than 32-bits the pattern is repeated for every 32-bits.
//
// pixels.r = MSB [0, R1, 0, R0] LSB pixels where each pixel is 8-bits.
// pixels.g = MSB [0, G1, 0, G0] LSB pixels where each pixel is 8-bits.
// pixels.b = MSB [0, B1, 0, B0] LSB pixels where each pixel is 8-bits.
//
// Stores 2x RGB565 pixels for every 32-bits.
static inline void vrgb_pixels_store_rgb565(uint16_t *p, uint32_t x, vrgb_pixels_t pixels, v128_predicate_t pred) {
    vstr_u16_pred(p + x, vrgb_pixels_to_rgb565(pixels), pred);
}

// In the case of vectors larger than 32-bits the pattern is repeated for every 32-bits.
//
// pixels.r = MSB [0, R1, 0, R0] LSB pixels where each pixel is 8-bits.
// pixels.g = MSB [0, G1, 0, G0] LSB pixels where each pixel is 8-bits.
// pixels.b = MSB [0, B1, 0, B0] LSB pixels where each pixel is 8-bits.
//
// Stores 2x binary pixels for every 32-bits.
static inline void vrgb_pixels_store_binary(uint32_t *p, uint32_t x, vrgb_pixels_t pixels, v128_predicate_t pred) {
    v128_t binary = vand_u32(vlsr_u32(vrgb_pixels_to_grayscale(pixels), 7), vdup_u16(1));

    // Turn the binary pixels that are in each 16-bit lane into a binary number which effectively
    // concatenates them all together.
    //
    // E.g. [bN*(1<<N) + ... + b1*(1<<1) + b0*(1<<0)] -> bN...b1b0
    //
    // The signed version vmladav is used on purpose since it has access to __SMUAD on ARMv7.
    uint32_t bits = vmladav_s16(binary, vshl_u16(vdup_u16(1), vidup_u16(0, 1)));

    uint32_t index = x >> 5;
    uint32_t offset = x & 0x1f;
    uint32_t remaining = 32 - offset;
    uint32_t count = vpredicate_16_get_n(pred);
    uint32_t min = (remaining < count) ? remaining : count;
    uint32_t mask = (1 << min) - 1;

    uint32_t v = p[index];
    v = (v & ~(mask << offset)) | ((bits & mask) << offset);
    p[index] = v;

    if (count > min) {
        mask = (1 << (count - min)) - 1;

        v = p[index + 1];
        v = (v & ~mask) | (bits & mask);
        p[index + 1] = v;
    }
}
