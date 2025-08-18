#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define global         static
#define internal       static

#define true    1
#define false   0

#define Assert(s) do{if(!(s)){*(int*)0=0;}}while(0)
#define DebugLog(m, ...) do{printf((m "\n"),##__VA_ARGS__);}while(0)
#define DebugLogCrash(m, ...) do{DebugLog(m,##__VA_ARGS__);Assert(0);}while(0)

#define SinF                       sinf
#define CosF                       cosf
#define TanF                       tanf
#define ATan2F                     atan2f
#define PowF                       powf
#define FModF                      fmodf
#define AbsoluteValue              fabsf
#define AbsoluteValueI             abs
#define SquareRoot                 sqrtf
#define Log2F                      log2f
#define MemoryCopy                 memcpy
#define MemorySet                  memset
#define MemoryMove                 memmove
#define StringCopy                 strcpy
#define StringCopyN                strncpy
#define CStringCompare             strcmp
#define CalculateCStringLength(s)  ((u32)strlen(s))
#define CStringToI32(s)            ((i32)atoi(s))
#define CStringToF32(s)            ((f32)atof(s))
#define MinValue(a, b)             (((a) < (b)) ? (a) : (b))
#define MaxValue(a, b)             (((a) > (b)) ? (a) : (b))
#define ClampValue(v, lo, hi)      (((v) < (lo)) ? (lo) : (((v) > (hi)) ? (hi) : (v)))
#define ArraySize(a)               (sizeof(a) / sizeof((a)[0]))

#define Bytes(n)      (n)
#define Kilobytes(n)  (Bytes(n)     * 1024)
#define Megabytes(n)  (Kilobytes(n) * 1024)
#define Gigabytes(n)  (Megabytes(n) * 1024)

#define PI  3.1415926535897
#define PIf 3.1415926535897f
#define ONE_OVER_SQUARE_ROOT_OF_TWO_PI  0.3989422804
#define ONE_OVER_SQUARE_ROOT_OF_TWO_PIf 0.3989422804f
#define EULERS_NUMBER  2.7182818284590452353602874713527
#define EULERS_NUMBERf 2.7182818284590452353602874713527f

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint8_t  b8;
typedef uint16_t b16;
typedef uint32_t b32;
typedef uint64_t b64;

typedef float    f32;
typedef double   f64;

internal f32
MaximumF32(f32 a, f32 b)
{
    return a > b ? a : b;
}

typedef struct v2
{
    f32 x;
    f32 y;
}
v2;

typedef union v3
{
    struct
    {
        f32 x;
        f32 y;
        f32 z;
    };
    
    struct
    {
        f32 r;
        f32 g;
        f32 b;
    };
    
    f32 elements[3];
}
v3;

typedef union v4
{
    struct
    {
        f32 x;
        f32 y;
		
        union
        {
            struct
            {
                f32 z;
                
                union
                {
                    f32 w;
                    f32 radius;
                };
            };
            struct
            {
                f32 width;
                f32 height;
            };
        };
    };
    
    struct
    {
        v3 xyz;
        f32 _unused;
    };
    
    struct
    {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    };
    
    f32 elements[4];
}
v4;

internal v2
V2Init(f32 x, f32 y)
{
    v2 v = { x, y };
    return v;
}

#define v2(x, y) V2Init(x, y)

internal v3
V3Init(f32 x, f32 y, f32 z)
{
    v3 v = { x, y, z };
    return v;
}

#define v3(x, y, z) V3Init(x, y, z)

internal f32
V2Dot(v2 a, v2 b)
{
    return a.x*b.x + a.y*b.y;
}

internal f32
V2LengthSquared(v2 v)
{
    return v.x*v.x + v.y*v.y;
}

internal v2
V2Normalize(v2 v)
{
    f32 length = SquareRoot(V2LengthSquared(v));
    v.x /= length;
    v.y /= length;
	
    return v;
}

internal v3
V3AddV3(v3 a, v3 b)
{
    v3 c = { a.x + b.x, a.y + b.y, a.z + b.z };
    return c;
}

internal v3
V3MinusV3(v3 a, v3 b)
{
    v3 c = { a.x - b.x, a.y - b.y, a.z - b.z };
    return c;
}

internal v3
V3MultiplyF32(v3 v, f32 f)
{
    v.x *= f;
    v.y *= f;
    v.z *= f;
	
    return v;
}

internal f32
V3LengthSquared(v3 a)
{
    return a.x*a.x + a.y*a.y + a.z*a.z;
}

internal f32
V3Length(v3 a)
{
    return SquareRoot(V3LengthSquared(a));
}

internal v3
V3Normalize(v3 v)
{
    f32 length = V3Length(v);
    
	v3 result = {
        v.x / length,
        v.y / length,
        v.z / length,
    };
	
    return result;
}

internal f32
V3Dot(v3 a, v3 b)
{
    f32 dot =
        a.x * b.x + 
        a.y * b.y +
        a.z * b.z;
	
    return dot;
}

internal v3
V3Cross(v3 a, v3 b)
{
    v3 result = {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x,
    };
	
    return result;
}

internal float
MinimumInV3(v3 v)
{
    float minimum = v.x;
    
	if(v.y < minimum) { minimum = v.y; }
    if(v.z < minimum) { minimum = v.z; }
	
    return minimum;
}

internal float
MaximumInV3(v3 v)
{
    float maximum = v.x;
	
    if(v.y > maximum) { maximum = v.y; }
    if(v.z > maximum) { maximum = v.z; }
    
	return maximum;
}

internal v4
V4Init(f32 x, f32 y, f32 z, f32 w)
{
    v4 v = { x, y, z, w };
    return v;
}
#define v4(x, y, z, w) V4Init(x, y, z, w)
#define v4u(x) v4(x, x, x, x)

internal b32
V4RectHasPoint(v4 r, v2 p)
{
    return(p.x >= r.x && p.x <= r.x + r.width &&
           p.y >= r.y && p.y <= r.y + r.height);
}

typedef struct v2i
{
    i32 x;
    i32 y;
}
v2i;

typedef union v3i
{
    struct
    {
        i32 x;
        i32 y;
        i32 z;
    };
    
    struct
    {
        i32 r;
        i32 g;
        i32 b;
    };
    
    i32 elements[3];
}
v3i;

typedef union v4i
{
    struct
    {
        i32 x;
        i32 y;
        i32 z;
        i32 w;
    };
    
    struct
    {
        i32 r;
        i32 g;
        i32 b;
        i32 a;
    };
    
    i32 elements[4];
}
v4i;

internal v2i
V2IInit(i32 x, i32 y)
{
    v2i v = { x, y };
    return v;
}

#define v2i(x, y) V2IInit(x, y)

internal v3i
V3IInit(i32 x, i32 y, i32 z)
{
    v3i v = { x, y, z };
    return v;
}

#define v3i(x, y, z) V3IInit(x, y, z)

internal v4i
V4IInit(i32 x, i32 y, i32 z, i32 w)
{
    v4i v = { x, y, z, w };
    return v;
}

#define v4i(x, y, z, w) V4IInit(x, y, z, w)

typedef struct m4
{
    f32 elements[4][4];
}
m4;

internal m4
M4InitD(f32 dia)
{
    m4 m = {
        {
            { dia, 0.f, 0.f, 0.f },
            { 0.f, dia, 0.f, 0.f },
            { 0.f, 0.f, dia, 0.f },
            { 0.f, 0.f, 0.f, dia },
        }
    };
	
    return m;
}

#define m4(d) M4InitD(d)

internal m4
M4MultiplyM4(m4 a, m4 b)
{
    m4 c = {0};
    
    for(i32 j = 0; j < 4; ++j)
    {
        for(i32 i = 0; i < 4; ++i)
        {
            c.elements[i][j] = (a.elements[0][j]*b.elements[i][0] +
                                a.elements[1][j]*b.elements[i][1] +
                                a.elements[2][j]*b.elements[i][2] +
                                a.elements[3][j]*b.elements[i][3]);
        }
    }
    
    return c;
}

internal m4
M4MultiplyF32(m4 a, f32 b)
{
    for(i32 j = 0; j < 4; ++j)
    {
        for(i32 i = 0; i < 4; ++i)
        {
            a.elements[i][j] *= b;
        }
    }
    
    return a;
}

internal f32
V4Dot(v4 a, v4 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

internal v4
V4AddV4(v4 a, v4 b)
{
    v4 c = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    return c;
}

internal v4
V4MinusV4(v4 a, v4 b)
{
    v4 c = { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
    return c;
}

internal v4
V4MultiplyF32(v4 a, f32 f)
{
    v4 c = { a.x * f, a.y * f, a.z * f, a.w * f };
    return c;
}

internal v4
V4MultiplyV4(v4 a, v4 b)
{
    v4 c = { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
    return c;
}

internal v4
V4MultiplyM4(v4 v, m4 m)
{
    v4 result = {0};
    
    for(i32 i = 0; i < 4; ++i)
    {
        result.elements[i] = (v.elements[0]*m.elements[0][i] +
                              v.elements[1]*m.elements[1][i] +
                              v.elements[2]*m.elements[2][i] +
                              v.elements[3]*m.elements[3][i]);
    }
    
    return result;
}

internal m4
M4TranslateV3(v3 translation)
{
    m4 result = m4(1.f);
	
    result.elements[3][0] = translation.x;
    result.elements[3][1] = translation.y;
    result.elements[3][2] = translation.z;
	
    return result;
}

internal m4
M4ScaleV3(v3 scale)
{
    m4 result = m4(1.f);
	
    result.elements[0][0] = scale.x;
    result.elements[1][1] = scale.y;
    result.elements[2][2] = scale.z;
	
    return result;
}

internal m4
M4Perspective(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z)
{
    m4 result = {0};
	
    f32 tan_theta_over_2 = TanF((fov / 360.f) * PIf);
    result.elements[0][0] = 1.f / tan_theta_over_2;
    result.elements[1][1] = aspect_ratio / tan_theta_over_2;
    result.elements[2][3] = -1.f;
    result.elements[2][2] = (near_z + far_z) / (near_z - far_z);
    result.elements[3][2] = (2.f * near_z * far_z) / (near_z - far_z);
    result.elements[3][3] = 0.f;
	
    return result;
}

internal m4
M4Orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_depth, f32 far_depth)
{
    m4 result = {0};
    
    result.elements[0][0] = 2.f / (right - left);
    result.elements[1][1] = 2.f / (top - bottom);
    result.elements[2][2] = -2.f / (far_depth - near_depth);
    result.elements[3][3] = 1.f;
    result.elements[3][0] = (left + right) / (left - right);
    result.elements[3][1] = (bottom + top) / (bottom - top);
    result.elements[3][2] = (far_depth + near_depth) / (near_depth - far_depth);
    
    return result;
}

internal m4
M4LookAt(v3 eye, v3 center, v3 up)
{
    m4 result = {0};
    
    v3 f = V3Normalize(V3MinusV3(center, eye));
    v3 s = V3Normalize(V3Cross(f, up));
    v3 u = V3Cross(s, f);
    
    result.elements[0][0] = s.x;
    result.elements[0][1] = u.x;
    result.elements[0][2] = -f.x;
    result.elements[0][3] = 0.0f;
    
    result.elements[1][0] = s.y;
    result.elements[1][1] = u.y;
    result.elements[1][2] = -f.y;
    result.elements[1][3] = 0.0f;
    
    result.elements[2][0] = s.z;
    result.elements[2][1] = u.z;
    result.elements[2][2] = -f.z;
    result.elements[2][3] = 0.0f;
    
    result.elements[3][0] = -V3Dot(s, eye);
    result.elements[3][1] = -V3Dot(u, eye);
    result.elements[3][2] = V3Dot(f, eye);
    result.elements[3][3] = 1.0f;
    
    return result;
}

internal m4
M4Inverse(m4 m)
{
    f32 coef00 = m.elements[2][2] * m.elements[3][3] - m.elements[3][2] * m.elements[2][3];
    f32 coef02 = m.elements[1][2] * m.elements[3][3] - m.elements[3][2] * m.elements[1][3];
    f32 coef03 = m.elements[1][2] * m.elements[2][3] - m.elements[2][2] * m.elements[1][3];
    f32 coef04 = m.elements[2][1] * m.elements[3][3] - m.elements[3][1] * m.elements[2][3];
    f32 coef06 = m.elements[1][1] * m.elements[3][3] - m.elements[3][1] * m.elements[1][3];
    f32 coef07 = m.elements[1][1] * m.elements[2][3] - m.elements[2][1] * m.elements[1][3];
    f32 coef08 = m.elements[2][1] * m.elements[3][2] - m.elements[3][1] * m.elements[2][2];
    f32 coef10 = m.elements[1][1] * m.elements[3][2] - m.elements[3][1] * m.elements[1][2];
    f32 coef11 = m.elements[1][1] * m.elements[2][2] - m.elements[2][1] * m.elements[1][2];
    f32 coef12 = m.elements[2][0] * m.elements[3][3] - m.elements[3][0] * m.elements[2][3];
    f32 coef14 = m.elements[1][0] * m.elements[3][3] - m.elements[3][0] * m.elements[1][3];
    f32 coef15 = m.elements[1][0] * m.elements[2][3] - m.elements[2][0] * m.elements[1][3];
    f32 coef16 = m.elements[2][0] * m.elements[3][2] - m.elements[3][0] * m.elements[2][2];
    f32 coef18 = m.elements[1][0] * m.elements[3][2] - m.elements[3][0] * m.elements[1][2];
    f32 coef19 = m.elements[1][0] * m.elements[2][2] - m.elements[2][0] * m.elements[1][2];
    f32 coef20 = m.elements[2][0] * m.elements[3][1] - m.elements[3][0] * m.elements[2][1];
    f32 coef22 = m.elements[1][0] * m.elements[3][1] - m.elements[3][0] * m.elements[1][1];
    f32 coef23 = m.elements[1][0] * m.elements[2][1] - m.elements[2][0] * m.elements[1][1];
    
    v4 fac0 = { coef00, coef00, coef02, coef03 };
    v4 fac1 = { coef04, coef04, coef06, coef07 };
    v4 fac2 = { coef08, coef08, coef10, coef11 };
    v4 fac3 = { coef12, coef12, coef14, coef15 };
    v4 fac4 = { coef16, coef16, coef18, coef19 };
    v4 fac5 = { coef20, coef20, coef22, coef23 };
    
    v4 vec0 = { m.elements[1][0], m.elements[0][0], m.elements[0][0], m.elements[0][0] };
    v4 vec1 = { m.elements[1][1], m.elements[0][1], m.elements[0][1], m.elements[0][1] };
    v4 vec2 = { m.elements[1][2], m.elements[0][2], m.elements[0][2], m.elements[0][2] };
    v4 vec3 = { m.elements[1][3], m.elements[0][3], m.elements[0][3], m.elements[0][3] };
    
    v4 inv0 = V4AddV4(V4MinusV4(V4MultiplyV4(vec1, fac0), V4MultiplyV4(vec2, fac1)), V4MultiplyV4(vec3, fac2));
    v4 inv1 = V4AddV4(V4MinusV4(V4MultiplyV4(vec0, fac0), V4MultiplyV4(vec2, fac3)), V4MultiplyV4(vec3, fac4));
    v4 inv2 = V4AddV4(V4MinusV4(V4MultiplyV4(vec0, fac1), V4MultiplyV4(vec1, fac3)), V4MultiplyV4(vec3, fac5));
    v4 inv3 = V4AddV4(V4MinusV4(V4MultiplyV4(vec0, fac2), V4MultiplyV4(vec1, fac4)), V4MultiplyV4(vec2, fac5));
    
    v4 sign_a = { +1, -1, +1, -1 };
    v4 sign_b = { -1, +1, -1, +1 };
    
    m4 inverse = {0};
    for(i32 i = 0; i < 4; ++i)
    {
        inverse.elements[0][i] = inv0.elements[i] * sign_a.elements[i];
        inverse.elements[1][i] = inv1.elements[i] * sign_b.elements[i];
        inverse.elements[2][i] = inv2.elements[i] * sign_a.elements[i];
        inverse.elements[3][i] = inv3.elements[i] * sign_b.elements[i];
    }
    
    v4 row0 = { inverse.elements[0][0], inverse.elements[1][0], inverse.elements[2][0], inverse.elements[3][0] };
    v4 m0 = { m.elements[0][0], m.elements[0][1], m.elements[0][2], m.elements[0][3] };
    v4 dot0 = V4MultiplyV4(m0, row0);
    f32 dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);
    
    f32 one_over_det = 1 / dot1;
    
    return M4MultiplyF32(inverse, one_over_det);
}

internal m4
M4RemoveRotation(m4 mat)
{
    v3 scale = {
        V3Length(v3(mat.elements[0][0], mat.elements[0][1], mat.elements[0][2])),
        V3Length(v3(mat.elements[1][0], mat.elements[1][1], mat.elements[1][2])),
        V3Length(v3(mat.elements[2][0], mat.elements[2][1], mat.elements[2][2])),
    };
    
    mat.elements[0][0] = scale.x;
    mat.elements[1][0] = 0.f;
    mat.elements[2][0] = 0.f;
    
    mat.elements[0][1] = 0.f;
    mat.elements[1][1] = scale.y;
    mat.elements[2][1] = 0.f;
    
    mat.elements[0][2] = 0.f;
    mat.elements[1][2] = 0.f;
    mat.elements[2][2] = scale.z;
    
    return mat;
}

internal m4
M4RotateAxis(f32 angle, v3 axis)
{
    m4 result = m4(1.f);
    
    axis = V3Normalize(axis);
    
    f32 sin_theta = SinF(angle);
    f32 cos_theta = CosF(angle);
    f32 cos_inv   = 1.0f - cos_theta;
    
    result.elements[0][0] = (axis.x * axis.x * cos_inv) +           cos_theta;
    result.elements[0][1] = (axis.x * axis.y * cos_inv) + (axis.z * sin_theta);
    result.elements[0][2] = (axis.x * axis.z * cos_inv) - (axis.y * sin_theta);
    
    result.elements[1][0] = (axis.y * axis.x * cos_inv) - (axis.z * sin_theta);
    result.elements[1][1] = (axis.y * axis.y * cos_inv) +           cos_theta;
    result.elements[1][2] = (axis.y * axis.z * cos_inv) + (axis.x * sin_theta);
    
    result.elements[2][0] = (axis.z * axis.x * cos_inv) + (axis.y * sin_theta);
    result.elements[2][1] = (axis.z * axis.y * cos_inv) - (axis.x * sin_theta);
    result.elements[2][2] = (axis.z * axis.z * cos_inv) +           cos_theta;
    
    return result;
}

typedef struct String8
{
	u8 *str;
	u64 size;
}
String8;

internal String8
String8Init(u8 *str, u64 size)
{
	String8 s = {0};
	s.str = str;
	s.size = size;
	
	return s;
}

#define str8(s) String8Init((u8 *)(s), sizeof(s) - 1)

internal b32
CharIsWhitespace(char c)
{
    return c <= 32;
}

internal b32
CharIsAlpha(char c)
{
    return ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z'));
}

internal b32
CharIsDigit(char c)
{
    return (c >= '0' && c <= '9');
}

internal int
CharToLower(u8 c)
{
    if(c >= 'A' && c <= 'Z')
    {
        c += 32;
    }
	
    return c;
}

internal int
CharToUpper(u8 c)
{
    if(c >= 'a' && c <= 'z')
    {
        c -= 32;
    }
	
    return c;
}

// NOTE(kp): FNV-1a 64-bit hash.
internal u64
HashBytesGeneric(const void *key, u64 length)
{
    const u8 *data = (const u8 *)key;
	u64 hash = 1469598103934665603ULL;
	
    for(u64 i = 0; i < length; i++)
	{
        hash ^= (u64)data[i];
        hash *= 1099511628211ULL;
    }
	
    return hash;
}

// NOTE(kp): FNV-1a 64-bit hash.
internal u64
HashBytesGenericCombine(u64 start, const void *key, u64 length)
{
    const u8 *data = (const u8 *)key;
	u64 hash = start;
	
    for(u64 i = 0; i < length; i++)
	{
        hash ^= (u64)data[i];
        hash *= 1099511628211ULL;
    }
	
    return hash;
}

internal u64
HashCString(char *string)
{
	u64 length = 0;
	while(string[length] != '\0') length++;
	return HashBytesGeneric(string, length);
}
