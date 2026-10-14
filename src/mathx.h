#ifndef MATHX_H
#define MATHX_H

#include <math.h>

#define MX_PI 3.14159265358979323846f

typedef struct { float x, y, z; } vec3;

/* Column-major 4x4 matrix: element (row r, col c) is m[c * 4 + r]. */
typedef struct { float m[16]; } mat4;

static inline float mx_radians(float deg) { return deg * (MX_PI / 180.0f); }
static inline float mx_clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static inline vec3 v3(float x, float y, float z) { vec3 r = { x, y, z }; return r; }
static inline vec3 v3_add(vec3 a, vec3 b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline vec3 v3_sub(vec3 a, vec3 b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline vec3 v3_scale(vec3 a, float s) { return v3(a.x * s, a.y * s, a.z * s); }
static inline float v3_dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline float v3_len(vec3 a) { return sqrtf(v3_dot(a, a)); }

static inline vec3 v3_cross(vec3 a, vec3 b)
{
    return v3(a.y * b.z - a.z * b.y,
              a.z * b.x - a.x * b.z,
              a.x * b.y - a.y * b.x);
}

static inline vec3 v3_norm(vec3 a)
{
    float len = v3_len(a);
    return len > 0.0f ? v3_scale(a, 1.0f / len) : a;
}

static inline mat4 mat4_identity(void)
{
    mat4 r = { { 0 } };
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

static inline mat4 mat4_mul(mat4 a, mat4 b)
{
    mat4 r;
    for (int c = 0; c < 4; c++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++)
                sum += a.m[k * 4 + row] * b.m[c * 4 + k];
            r.m[c * 4 + row] = sum;
        }
    }
    return r;
}

static inline mat4 mat4_translate(vec3 t)
{
    mat4 r = mat4_identity();
    r.m[12] = t.x;
    r.m[13] = t.y;
    r.m[14] = t.z;
    return r;
}

static inline mat4 mat4_scale(vec3 s)
{
    mat4 r = mat4_identity();
    r.m[0] = s.x;
    r.m[5] = s.y;
    r.m[10] = s.z;
    return r;
}

/* OpenGL-style perspective projection (clip z in [-1, 1]). fovy in radians. */
static inline mat4 mat4_perspective(float fovy, float aspect, float znear, float zfar)
{
    float f = 1.0f / tanf(fovy * 0.5f);
    mat4 r = { { 0 } };
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (zfar + znear) / (znear - zfar);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * zfar * znear) / (znear - zfar);
    return r;
}

static inline mat4 mat4_look_at(vec3 eye, vec3 center, vec3 up)
{
    vec3 f = v3_norm(v3_sub(center, eye));
    vec3 s = v3_norm(v3_cross(f, up));
    vec3 u = v3_cross(s, f);
    mat4 r = mat4_identity();
    r.m[0] = s.x;  r.m[4] = s.y;  r.m[8]  = s.z;
    r.m[1] = u.x;  r.m[5] = u.y;  r.m[9]  = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -v3_dot(s, eye);
    r.m[13] = -v3_dot(u, eye);
    r.m[14] = v3_dot(f, eye);
    return r;
}

#endif
