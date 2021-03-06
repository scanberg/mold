#ifndef _VEC_MATH_H_
#define _VEC_MATH_H_

#ifndef VEC_MATH_USE_SSE_H
#define VEC_MATH_USE_SSE_H 1
#endif

#ifdef VEC_MATH_USE_SSE_H
#include <xmmintrin.h>
#endif

#include "md_compiler.h"
#if MD_COMPILER_MSVC
#pragma warning( disable : 4201 ) // nameless structs
#endif

#include <math.h>

typedef union vec3 {
    struct {
        float x, y, z;  
    };
    float elem[3];
} vec3_t;

typedef union vec4 {
    struct {
        float x, y, z, w;
    };
    float elem[4];
#if VEC_MATH_USE_SSE_H
    __m128 mm128;
#endif
} vec4_t;

typedef union mat3 {
    float elem[3][3];
    vec3_t col[3];
} mat3_t;

typedef union mat4 {
    float elem[4][4];
    vec4_t col[4];
} mat4_t;

// VEC3 OPERATIONS
static inline vec3_t vec3_add(vec3_t a, vec3_t b) {
    vec3_t res = {a.x + b.x, a.y + b.y, a.z + b.z};
    return res;
}

static inline vec3_t vec3_add_f(vec3_t a, float f) {
    vec3_t res = {a.x + f, a.y + f, a.z + f};
    return res;
}

static inline vec3_t vec3_sub(vec3_t a, vec3_t b) {
    vec3_t res = {a.x - b.x, a.y - b.y, a.z - b.z};
    return res;
}

static inline vec3_t vec3_sub_f(vec3_t a, float f) {
    vec3_t res = {a.x - f, a.y - f, a.z - f};
    return res;
}

static inline vec3_t vec3_mul(vec3_t a, vec3_t b) {
    vec3_t res = {a.x * b.x, a.y * b.y, a.z * b.z};
    return res;
}

static inline vec3_t vec3_mul_f(vec3_t a, float f) {
    vec3_t res = {a.x * f, a.y * f, a.z * f};
    return res;
}

static inline vec3_t vec3_div(vec3_t a, vec3_t b) {
    vec3_t res = {a.x / b.x, a.y / b.y, a.z / b.z};
    return res;
}

static inline vec3_t vec3_div_f(vec3_t a, float f) {
    vec3_t res = {a.x / f, a.y / f, a.z / f};
    return res;
}

static inline vec3_t vec3_cross(vec3_t a, vec3_t b) {
    vec3_t res = {
        a.y * b.z - b.y * a.z,
        a.z * b.x - b.z * a.x,
        a.x * b.y - b.x * a.y};
    return res;
}

static inline float vec3_dot(vec3_t a, vec3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float vec3_length(vec3_t v) {
    float l2 = vec3_dot(v,v);
    return sqrtf(l2);
}

static inline float vec3_dist(vec3_t a, vec3_t b) {
    vec3_t d = vec3_sub(a, b);
    return vec3_length(d);
}

static inline vec3_t vec3_normalize(vec3_t v) {
    float len = vec3_length(v);
    if (len > 1.0e-5) {
        vec3_t res = {v.x / len, v.y / len, v.z / len};
        return res;
    }

    vec3_t res = {0, 0, 0};
    return res;
}

// VEC4 OPERATIONS
static inline vec4_t vec4_mul(const vec4_t a, const vec4_t b) {
    vec4_t c = {0};
#if VEC_MATH_USE_SSE_H
    c.mm128 = _mm_mul_ps(a.mm128, b.mm128);
#else
    c.x = a.x * b.x;
    c.y = a.y * b.y;
    c.z = a.z * b.z;
    c.w = a.w * b.w;
#endif
    return c;
}

static inline vec4_t vec4_mul_f(const vec4_t a, const float s) {
    vec4_t c = {0};
#if VEC_MATH_USE_SSE_H
    c.mm128 = _mm_mul_ps(a.mm128, _mm_set1_ps(s));
#else
    c.x = a.x * s;
    c.y = a.y * s;
    c.z = a.z * s;
    c.w = a.w * s;
#endif
    return c;
}

static inline vec4_t vec4_div(const vec4_t a, const vec4_t b) {
    vec4_t c = {0};
#if VEC_MATH_USE_SSE_H
    c.mm128 = _mm_div_ps(a.mm128, b.mm128);
#else
    c.x = a.x / b.x;
    c.y = a.y / b.y;
    c.z = a.z / b.z;
    c.w = a.w / b.w;
#endif
    return c;
}

static inline vec4_t vec4_div_f(const vec4_t a, const float s) {
    vec4_t c = {0};
#if VEC_MATH_USE_SSE_H
    c.mm128 = _mm_div_ps(a.mm128, _mm_set1_ps(s));
#else
    c.x = a.x / s;
    c.y = a.y / s;
    c.z = a.z / s;
    c.w = a.w / s;
#endif
    return c;
}

static inline vec4_t vec4_add(const vec4_t a, const vec4_t b) {
    vec4_t c = {0};
#if VEC_MATH_USE_SSE_H
    c.mm128 = _mm_add_ps(a.mm128, b.mm128);
#else
    c.x = a.x + b.x;
    c.y = a.y + b.y;
    c.z = a.z + b.z;
    c.w = a.w + b.w;
#endif
    return c;
}

static inline vec4_t vec4_add_f(const vec4_t a, const float s) {
    vec4_t c = {0};
#if VEC_MATH_USE_SSE_H
    c.mm128 = _mm_add_ps(a.mm128, _mm_set1_ps(s));
#else
    c.x = a.x + s;
    c.y = a.y + s;
    c.z = a.z + s;
    c.w = a.w + s;
#endif
    return c;
}

static inline vec4_t vec4_sub(const vec4_t a, const vec4_t b) {
    vec4_t c = {0};
#if VEC_MATH_USE_SSE_H
    c.mm128 = _mm_sub_ps(a.mm128, b.mm128);
#else
    c.x = a.x - b.x;
    c.y = a.y - b.y;
    c.z = a.z - b.z;
    c.w = a.w - b.w;
#endif
    return c;
}

static inline vec4_t vec4_sub_f(const vec4_t a, const float s) {
    vec4_t c = {0};
#if VEC_MATH_USE_SSE_H
    c.mm128 = _mm_sub_ps(a.mm128, _mm_set1_ps(s));
#else
    c.x = a.x - s;
    c.y = a.y - s;
    c.z = a.z - s;
    c.w = a.w - s;
#endif
    return c;
}

#if VEC_MATH_USE_SSE_H
static inline __m128 md_linear_combine_sse(__m128 a, mat4_t B) {
    __m128 res;
    res = _mm_mul_ps(_mm_shuffle_ps(a, a, 0x00), B.col[0].mm128);
    res = _mm_add_ps(res, _mm_mul_ps(_mm_shuffle_ps(a, a, 0x55), B.col[1].mm128));
    res = _mm_add_ps(res, _mm_mul_ps(_mm_shuffle_ps(a, a, 0xaa), B.col[2].mm128));
    res = _mm_add_ps(res, _mm_mul_ps(_mm_shuffle_ps(a, a, 0xff), B.col[3].mm128));
    return res;
}
#endif

static inline mat4_t mat4_mul(const mat4_t A, const mat4_t B) {
    mat4_t C = {0};
#if VEC_MATH_USE_SSE_H
    C.col[0].mm128 = md_linear_combine_sse(B.col[0].mm128, A);
    C.col[1].mm128 = md_linear_combine_sse(B.col[1].mm128, A);
    C.col[2].mm128 = md_linear_combine_sse(B.col[2].mm128, A);
    C.col[3].mm128 = md_linear_combine_sse(B.col[3].mm128, A);
#else
#define MULT(col, row) \
    A.elem[0][row] * B.elem[col][0] + A.elem[1][row] * B.elem[col][1] + A.elem[2][row] * B.elem[col][2] + A.elem[3][row] * B.elem[col][3]
    C.elem[0][0] = MULT(0, 0);
    C.elem[0][1] = MULT(0, 1);
    C.elem[0][2] = MULT(0, 2);
    C.elem[0][3] = MULT(0, 3);

    C.elem[1][0] = MULT(1, 0);
    C.elem[1][1] = MULT(1, 1);
    C.elem[1][2] = MULT(1, 2);
    C.elem[1][3] = MULT(1, 3);

    C.elem[2][0] = MULT(2, 0);
    C.elem[2][1] = MULT(2, 1);
    C.elem[2][2] = MULT(2, 2);
    C.elem[2][3] = MULT(2, 3);

    C.elem[3][0] = MULT(3, 0);
    C.elem[3][1] = MULT(3, 1);
    C.elem[3][2] = MULT(3, 2);
    C.elem[3][3] = MULT(3, 3);
#undef MULT
#endif
    return C;
}

static inline vec4_t mat4_mul_vec4(const mat4_t M, const vec4_t V) {
    vec4_t R = {0};
#if VEC_MATH_USE_SSE_H
    R.mm128 = md_linear_combine_sse(V.mm128, M);
#else
    ASSERT(false);
#endif
    return R;
}

static inline mat4_t mat4_mul_f(const mat4_t M, float s) {
    mat4_t C = {0};
#if VEC_MATH_USE_SSE_H
    C.col[0] = vec4_mul_f(M.col[0], s);
    C.col[1] = vec4_mul_f(M.col[1], s);
    C.col[2] = vec4_mul_f(M.col[2], s);
    C.col[3] = vec4_mul_f(M.col[3], s);
#else
    C.elem[0][0] = M.elem[0][0] * s;
    C.elem[0][1] = M.elem[0][1] * s;
    C.elem[0][2] = M.elem[0][2] * s;
    C.elem[0][3] = M.elem[0][3] * s;

    C.elem[1][0] = M.elem[1][0] * s;
    C.elem[1][1] = M.elem[1][1] * s;
    C.elem[1][2] = M.elem[1][2] * s;
    C.elem[1][3] = M.elem[1][3] * s;

    C.elem[2][0] = M.elem[2][0] * s;
    C.elem[2][1] = M.elem[2][1] * s;
    C.elem[2][2] = M.elem[2][2] * s;
    C.elem[2][3] = M.elem[2][3] * s;

    C.elem[3][0] = M.elem[3][0] * s;
    C.elem[3][1] = M.elem[3][1] * s;
    C.elem[3][2] = M.elem[3][2] * s;
    C.elem[3][3] = M.elem[3][3] * s;
#endif
    return C;
}

static inline mat4_t mat4_ident() {
    return (mat4_t) {.col[0] = {1,0,0,0}, .col[1] = {0,1,0,0}, .col[2] = {0,0,1,0}, .col[3] = {0,0,0,1}};
}

static inline mat4_t mat4_inverse(const mat4_t M) {
    const float c00 = M.elem[2][2] * M.elem[3][3] - M.elem[3][2] * M.elem[2][3];
    const float c02 = M.elem[1][2] * M.elem[3][3] - M.elem[3][2] * M.elem[1][3];
    const float c03 = M.elem[1][2] * M.elem[2][3] - M.elem[2][2] * M.elem[1][3];

    const float c04 = M.elem[2][1] * M.elem[3][3] - M.elem[3][1] * M.elem[2][3];
    const float c06 = M.elem[1][1] * M.elem[3][3] - M.elem[3][1] * M.elem[1][3];
    const float c07 = M.elem[1][1] * M.elem[2][3] - M.elem[2][1] * M.elem[1][3];

    const float c08 = M.elem[2][1] * M.elem[3][2] - M.elem[3][1] * M.elem[2][2];
    const float c10 = M.elem[1][1] * M.elem[3][2] - M.elem[3][1] * M.elem[1][2];
    const float c11 = M.elem[1][1] * M.elem[2][2] - M.elem[2][1] * M.elem[1][2];

    const float c12 = M.elem[2][0] * M.elem[3][3] - M.elem[3][0] * M.elem[2][3];
    const float c14 = M.elem[1][0] * M.elem[3][3] - M.elem[3][0] * M.elem[1][3];
    const float c15 = M.elem[1][0] * M.elem[2][3] - M.elem[2][0] * M.elem[1][3];

    const float c16 = M.elem[2][0] * M.elem[3][2] - M.elem[3][0] * M.elem[2][2];
    const float c18 = M.elem[1][0] * M.elem[3][2] - M.elem[3][0] * M.elem[1][2];
    const float c19 = M.elem[1][0] * M.elem[2][2] - M.elem[2][0] * M.elem[1][2];

    const float c20 = M.elem[2][0] * M.elem[3][1] - M.elem[3][0] * M.elem[2][1];
    const float c22 = M.elem[1][0] * M.elem[3][1] - M.elem[3][0] * M.elem[1][1];
    const float c23 = M.elem[1][0] * M.elem[2][1] - M.elem[2][0] * M.elem[1][1];

    const vec4_t f0 = {c00, c00, c02, c03};
    const vec4_t f1 = {c04, c04, c06, c07};
    const vec4_t f2 = {c08, c08, c10, c11};
    const vec4_t f3 = {c12, c12, c14, c15};
    const vec4_t f4 = {c16, c16, c18, c19};
    const vec4_t f5 = {c20, c20, c22, c23};

    const vec4_t v0 = {M.elem[1][0], M.elem[0][0], M.elem[0][0], M.elem[0][0]};
    const vec4_t v1 = {M.elem[1][1], M.elem[0][1], M.elem[0][1], M.elem[0][1]};
    const vec4_t v2 = {M.elem[1][2], M.elem[0][2], M.elem[0][2], M.elem[0][2]};
    const vec4_t v3 = {M.elem[1][3], M.elem[0][3], M.elem[0][3], M.elem[0][3]};

    const vec4_t i0 = vec4_add(vec4_sub(vec4_mul(v1, f0), vec4_mul(v2, f1)), vec4_mul(v3, f2));
    const vec4_t i1 = vec4_add(vec4_sub(vec4_mul(v0, f0), vec4_mul(v2, f3)), vec4_mul(v3, f4));
    const vec4_t i2 = vec4_add(vec4_sub(vec4_mul(v0, f1), vec4_mul(v1, f3)), vec4_mul(v3, f5));
    const vec4_t i3 = vec4_add(vec4_sub(vec4_mul(v0, f2), vec4_mul(v1, f4)), vec4_mul(v2, f5));

    const vec4_t sign_a = {+1, -1, +1, -1};
    const vec4_t sign_b = {-1, +1, -1, +1};

    mat4_t I = {0};
    I.col[0] = vec4_mul(i0, sign_a);
    I.col[1] = vec4_mul(i1, sign_b);
    I.col[2] = vec4_mul(i2, sign_a);
    I.col[3] = vec4_mul(i3, sign_b);

    const vec4_t row0 = {I.elem[0][0], I.elem[1][0], I.elem[2][0], I.elem[3][0]};
    const vec4_t dot0 = vec4_mul(M.col[0], row0);

    return mat4_mul_f(I, 1.0f / (dot0.x + dot0.y + dot0.z + dot0.w));
}

static inline mat4_t mat4_transpose(const mat4_t M) {
    mat4_t T;

#if VEC_MATH_USE_SSE_H
    T = M;
    _MM_TRANSPOSE4_PS(T.col[0].mm128, T.col[1].mm128, T.col[2].mm128, T.col[3].mm128);
#else
    T.elem[0][0] = M.elem[0][0];
    T.elem[0][1] = M.elem[1][0];
    T.elem[0][2] = M.elem[2][0];
    T.elem[0][3] = M.elem[3][0];

    T.elem[1][0] = M.elem[0][1];
    T.elem[1][1] = M.elem[1][1];
    T.elem[1][2] = M.elem[2][1];
    T.elem[1][3] = M.elem[3][1];

    T.elem[2][0] = M.elem[0][2];
    T.elem[2][1] = M.elem[1][2];
    T.elem[2][2] = M.elem[2][2];
    T.elem[2][3] = M.elem[3][2];

    T.elem[3][0] = M.elem[0][3];
    T.elem[3][1] = M.elem[1][3];
    T.elem[3][2] = M.elem[2][3];
    T.elem[3][3] = M.elem[3][3];
#endif
    return T;
}


#endif