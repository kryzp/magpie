#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <float.h>

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
#define ASinF                      asinf
#define ACosF                      acosf
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
#define EPSILON DBL_EPSILON
#define EPSILONf FLT_EPSILON

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
V3Init(f32 x, f32 y, f32 z)
{
    v3 v = { x, y, z };
    return v;
}

#define v3(x, y, z) V3Init(x, y, z)

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

internal f32
V3LengthSquared(v3 a)
{
	return V3Dot(a, a);
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
    return (p.x >= r.x && p.x <= r.x + r.width &&
			p.y >= r.y && p.y <= r.y + r.height);
}

internal f32
V4Dot(v4 a, v4 b)
{
    return (a.x * b.x +
			a.y * b.y +
			a.z * b.z +
			a.w * b.w);
}

internal v4
V4AddV4(v4 a, v4 b)
{
    return v4(a.x + b.x,
			  a.y + b.y,
			  a.z + b.z,
			  a.w + b.w);
}

internal v4
V4MinusV4(v4 a, v4 b)
{
    return v4(a.x - b.x,
			  a.y - b.y,
			  a.z - b.z,
			  a.w - b.w);
}

internal v4
V4MultiplyF32(v4 a, f32 f)
{
    return v4(a.x * f,
			  a.y * f,
			  a.z * f,
			  a.w * f);
}

internal v4
V4MultiplyV4(v4 a, v4 b)
{
    return v4(a.x * b.x,
			  a.y * b.y,
			  a.z * b.z,
			  a.w * b.w);
}

internal f32
V4LengthSquared(v4 v)
{
	return V4Dot(v, v);
}

internal f32
V4Length(v4 v)
{
	return SquareRoot(V4LengthSquared(v));
}

internal v4
QuatInitAngleAxis(f32 angle, v3 axis)
{
	v4 q = {0};
	
	q.w = CosF(angle * .5f);
	q.x = SinF(angle * .5f) * axis.x;
	q.y = SinF(angle * .5f) * axis.y;
	q.z = SinF(angle * .5f) * axis.z;
	
	return q;
}

internal v4
QuatInitEuler(f32 pitch, f32 yaw, f32 roll)
{
	f32 sr = SinF(roll  * .5f);
	f32 cr = CosF(roll  * .5f);
	f32 sp = SinF(pitch * .5f);
	f32 cp = CosF(pitch * .5f);
	f32 sy = SinF(yaw   * .5f);
	f32 cy = CosF(yaw   * .5f);
	
	return v4((sr * cp * cy) - (cr * sp * sy),
			  (cr * sp * cy) + (sr * cp * sy),
			  (cr * cp * sy) - (sr * sp * cy),
			  (cr * cp * cy) + (sr * sp * sy));
}

internal v3
QuatToEuler(v4 q)
{
	f32 t0 =           (2.f + ((q.w * q.x) + (q.y * q.z)));
	f32 t1 = 1.f -     (2.f * ((q.x * q.x) + (q.y * q.y)));
	f32 t2 = ClampValue(2.f * ((q.w * q.y) - (q.z * q.x)), -1.f, 1.f);
	f32 t3 =           (2.f * ((q.w * q.z) + (q.x * q.y)));
	f32 t4 = 1.f -     (2.f * ((q.y * q.y) + (q.z * q.z)));
	
	f32 pitch = ASinF(t2);
	f32 yaw   = ATan2F(t3, t4);
	f32 roll  = ATan2F(t0, t1);
	
	return v3(pitch, yaw, roll);
}

internal v4
QuatInverse(v4 q)
{
	v4 inverse = {0};
	
	f32 prod = V4LengthSquared(q);
	
	if(prod > EPSILONf)
	{
		inverse.x=-q.x / SquareRoot(prod);
		inverse.x=-q.y / SquareRoot(prod);
		inverse.x=-q.z / SquareRoot(prod);
		inverse.w= q.w / SquareRoot(prod);
	}
	else
	{
		inverse.x =-q.x;
		inverse.y =-q.y;
		inverse.z =-q.z;
		inverse.w = q.w;
	}
	
	return inverse;
}

typedef struct v2i
{
    i32 x;
    i32 y;
}
v2i;

internal v2i
V2IInit(i32 x, i32 y)
{
    v2i v = { x, y };
    return v;
}

#define v2i(x, y) V2IInit(x, y)

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

internal v3i
V3IInit(i32 x, i32 y, i32 z)
{
    v3i v = { x, y, z };
    return v;
}

#define v3i(x, y, z) V3IInit(x, y, z)

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

internal v4i
V4IInit(i32 x, i32 y, i32 z, i32 w)
{
    v4i v = { x, y, z, w };
    return v;
}

#define v4i(x, y, z, w) V4IInit(x, y, z, w)

// NOTE(kp): Column-major layout.
typedef union m4
{
    struct
	{
        f32 m00, m10, m20, m30;
        f32 m01, m11, m21, m31;
        f32 m02, m12, m22, m32;
        f32 m03, m13, m23, m33;
    };
	
    struct
	{
        f32 c0[4];
        f32 c1[4];
        f32 c2[4];
        f32 c3[4];
    };
}
m4;

internal m4
M4Init(f32 diag)
{
    m4 m = {
		diag, 0.f, 0.f, 0.f,
		0.f, diag, 0.f, 0.f,
		0.f, 0.f, diag, 0.f,
		0.f, 0.f, 0.f, diag
    };
	
    return m;
}

#define m4(d) M4Init(d)
#define m4c(m, __colindex) (((f32 *)(&m)) + (4*(__colindex)))

internal m4
M4MultiplyM4(m4 a, m4 b)
{
	m4 c = {0};
	
	for(i32 k = 0; k < 4; k++)
	{
		for(i32 i = 0; i < 4; i++)
		{
			for(i32 j = 0; j < 4; j++)
			{
				m4c(c, k)[i] += m4c(a, j)[i] * m4c(b, k)[j]; 
			}
		}
	}
	
	return c;
}

internal v4
M4MultiplyV4(m4 m, v4 v)
{
	v4 r = {0};
	
	for(i32 i = 0; i < 4; i++)
	{
		for(i32 j = 0; j < 4; j++)
		{
			r.elements[i] += m4c(m, j)[i] * v.elements[j];
		}
	}
	
	return r;
}

internal v3
M4MultiplyV3(m4 m, v3 v)
{
	v4 w = { v.x, v.y, v.z, 1.f };
	return M4MultiplyV4(m, w).xyz;
}

internal m4
M4MultiplyF32(m4 m, f32 f)
{
	m4 r = {0};
	
	for(i32 i = 0; i < 4; i++)
	{
		for(i32 j = 0; j < 4; j++)
		{
			m4c(r, i)[j] = m4c(m, i)[j] * f;
		}
	}
	
	return r;
}

internal m4
M4TranslateV3(v3 translation)
{
    m4 result = m4(1.f);
	
	result.m03 = translation.x;
	result.m13 = translation.y;
	result.m23 = translation.z;
	result.m33 = 1.f;
	
    return result;
}

internal m4
M4ScaleV3(v3 scale)
{
    m4 result = m4(1.f);
	
	result.m00 = scale.x;
	result.m11 = scale.y;
	result.m22 = scale.z;
	result.m33 = 1.f;
	
    return result;
}

internal m4
M4LookAt(v3 eye, v3 center, v3 up)
{
    m4 result = {0};
	
    v3 yaxis = V3Normalize(V3MinusV3(center, eye));
    v3 xaxis = V3Normalize(V3Cross(yaxis, up));
    v3 zaxis = V3Cross(xaxis, yaxis);
    
    result.m00 = xaxis.x;
    result.m01 = xaxis.y;
    result.m02 = xaxis.z;
    result.m03 = -V3Dot(xaxis, eye);
	
    result.m10 = yaxis.x;
    result.m11 = yaxis.y;
    result.m12 = yaxis.z;
    result.m13 = -V3Dot(yaxis, eye);
    
    result.m20 = zaxis.x;
    result.m21 = zaxis.y;
    result.m22 = zaxis.z;
    result.m23 = -V3Dot(zaxis, eye);
    
    result.m30 = 0.f;
    result.m31 = 0.f;
    result.m32 = 0.f;
    result.m33 = 1.f;
    
    return result;
}

internal m4
M4Perspective(f32 fov, f32 aspect, f32 z_near, f32 z_far)
{
    m4 result = {0};
	
    f32 f = TanF(fov / 360.f * PIf);
	
    result.m00 = f / aspect;
	result.m12 = f;
	result.m21 = (z_far + z_near) / (z_far - z_near);
	result.m23 =-(2.f * z_far * z_near) / (z_far - z_near);
	result.m31 = 1.f;
	
    return result;
}

internal m4
M4Orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 z_near, f32 z_far)
{
	m4 result = {0};
	
	result.m00 = 2.f / (right - left);
	result.m12 = 2.f / (top - bottom);
	result.m21 = 2.f / (z_far - z_near);
	
	result.m03 =-(right + left) / (right - left);
	result.m13 =-(top + bottom) / (top - bottom);
	result.m23 =-(z_far + z_near) / (z_far - z_near);
	result.m33 = 1.f;
	
	return result;
}

internal m4
M4Transpose(m4 m)
{
	m4 result = {0};
	
	for(i32 i = 0; i < 4; i++)
	{
		for(i32 j = 0; j < 4; j++)
		{
			m4c(result, i)[j] = m4c(m, j)[i];
		}
	}
	
	return result;
}

internal m4
M4Inverse(m4 m)
{
    f32 coef00 = m.m22 * m.m33 - m.m23 * m.m32;
    f32 coef02 = m.m21 * m.m33 - m.m23 * m.m31;
    f32 coef03 = m.m21 * m.m32 - m.m22 * m.m31;
    f32 coef04 = m.m12 * m.m33 - m.m13 * m.m32;
    f32 coef06 = m.m11 * m.m33 - m.m13 * m.m31;
    f32 coef07 = m.m11 * m.m32 - m.m12 * m.m31;
    f32 coef08 = m.m12 * m.m23 - m.m13 * m.m22;
    f32 coef10 = m.m11 * m.m23 - m.m13 * m.m21;
    f32 coef11 = m.m11 * m.m22 - m.m12 * m.m21;
    f32 coef12 = m.m02 * m.m33 - m.m03 * m.m32;
    f32 coef14 = m.m01 * m.m33 - m.m03 * m.m31;
    f32 coef15 = m.m01 * m.m32 - m.m02 * m.m31;
    f32 coef16 = m.m02 * m.m23 - m.m03 * m.m22;
    f32 coef18 = m.m01 * m.m23 - m.m03 * m.m21;
    f32 coef19 = m.m01 * m.m22 - m.m02 * m.m21;
    f32 coef20 = m.m02 * m.m13 - m.m03 * m.m12;
    f32 coef22 = m.m01 * m.m13 - m.m03 * m.m11;
    f32 coef23 = m.m01 * m.m12 - m.m02 * m.m11;
    
    v4 fac0 = { coef00, coef00, coef02, coef03 };
    v4 fac1 = { coef04, coef04, coef06, coef07 };
    v4 fac2 = { coef08, coef08, coef10, coef11 };
    v4 fac3 = { coef12, coef12, coef14, coef15 };
    v4 fac4 = { coef16, coef16, coef18, coef19 };
    v4 fac5 = { coef20, coef20, coef22, coef23 };
    
    v4 vec0 = { m.m01, m.m00, m.m00, m.m00 };
    v4 vec1 = { m.m11, m.m10, m.m10, m.m10 };
    v4 vec2 = { m.m21, m.m20, m.m20, m.m20 };
    v4 vec3 = { m.m31, m.m30, m.m30, m.m30 };
    
    v4 inv0 = V4AddV4(V4MinusV4(V4MultiplyV4(vec1, fac0), V4MultiplyV4(vec2, fac1)), V4MultiplyV4(vec3, fac2));
    v4 inv1 = V4AddV4(V4MinusV4(V4MultiplyV4(vec0, fac0), V4MultiplyV4(vec2, fac3)), V4MultiplyV4(vec3, fac4));
    v4 inv2 = V4AddV4(V4MinusV4(V4MultiplyV4(vec0, fac1), V4MultiplyV4(vec1, fac3)), V4MultiplyV4(vec3, fac5));
    v4 inv3 = V4AddV4(V4MinusV4(V4MultiplyV4(vec0, fac2), V4MultiplyV4(vec1, fac4)), V4MultiplyV4(vec2, fac5));
    
    v4 sign_a = { +1.f, -1.f, +1.f, -1.f };
    v4 sign_b = { -1.f, +1.f, -1.f, +1.f };
    
    m4 inverse = {0};
	
    for(i32 i = 0; i < 4; i++)
    {
        inverse.c0[i] = inv0.elements[i] * sign_a.elements[i];
        inverse.c1[i] = inv1.elements[i] * sign_b.elements[i];
        inverse.c2[i] = inv2.elements[i] * sign_a.elements[i];
        inverse.c3[i] = inv3.elements[i] * sign_b.elements[i];
    }
    
    v4 row0 = { inverse.m00, inverse.m01, inverse.m02, inverse.m03 };
    v4 m0 = { m.m00, m.m10, m.m20, m.m30 };
    v4 dot0 = V4MultiplyV4(m0, row0);
    f32 dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);
    
    f32 one_over_det = 1.f / dot1;
    
    return M4MultiplyF32(inverse, one_over_det);
}

internal m4
M4RemoveTranslation(m4 m)
{
	m.m03 = 0.f;
	m.m13 = 0.f;
	m.m23 = 0.f;
	m.m33 = 1.f;
	
	return m;
}

internal m4
M4RemoveRotation(m4 m)
{
    v3 scale = {
        V3Length(v3(m.m00, m.m10, m.m20)),
        V3Length(v3(m.m01, m.m11, m.m21)),
        V3Length(v3(m.m02, m.m12, m.m22)),
    };
    
    m.m00 = scale.x;
    m.m01 = 0.f;
    m.m02 = 0.f;
    
    m.m10 = 0.f;
    m.m11 = scale.y;
    m.m12 = 0.f;
    
    m.m20 = 0.f;
    m.m21 = 0.f;
    m.m22 = scale.z;
    
    return m;
}

internal m4
M4RotateAxis(f32 angle, v3 axis)
{
    m4 result = m4(1.f);
    
    axis = V3Normalize(axis);
    
    f32 sin_theta = SinF(angle);
    f32 cos_theta = CosF(angle);
    f32 cos_inv   = 1.f - cos_theta;
    
    result.m00 = (axis.x * axis.x * cos_inv) +           cos_theta;
    result.m10 = (axis.x * axis.y * cos_inv) + (axis.z * sin_theta);
    result.m20 = (axis.x * axis.z * cos_inv) - (axis.y * sin_theta);
    
    result.m01 = (axis.y * axis.x * cos_inv) - (axis.z * sin_theta);
    result.m11 = (axis.y * axis.y * cos_inv) +           cos_theta;
    result.m21 = (axis.y * axis.z * cos_inv) + (axis.x * sin_theta);
    
    result.m02 = (axis.z * axis.x * cos_inv) + (axis.y * sin_theta);
    result.m12 = (axis.z * axis.y * cos_inv) - (axis.x * sin_theta);
    result.m22 = (axis.z * axis.z * cos_inv) +           cos_theta;
    
    return result;
}

// NOTE(kp): Input quaternion must be normalized. 
internal m4
M4RotateQuat(v4 q)
{
	m4 result = m4(1.f);
	
	result.m00 = 1.f - 2.f * (q.y * q.y + q.z * q.z);
	result.m01 =       2.f * (q.x * q.y - q.z * q.w);
	result.m02 =       2.f * (q.x * q.z + q.y * q.w);
	
	result.m10 =       2.f * (q.x * q.y + q.z * q.w);
	result.m11 = 1.f - 2.f * (q.x * q.x + q.z * q.z);
	result.m12 =       2.f * (q.y * q.z - q.x * q.w);
	
	result.m20 =       2.f * (q.x * q.z - q.y * q.w);
	result.m21 =       2.f * (q.y * q.z + q.x * q.w);
	result.m22 = 1.f - 2.f * (q.x * q.x + q.y * q.y);
	
	return result;
}

internal m4
M4Transform(v3 position, v4 rotation, v3 scale, v3 origin)
{
	m4 result = m4(1.f);
	
	result = M4MultiplyM4(M4TranslateV3(V3MultiplyF32(origin, -1.f)), result);
	result = M4MultiplyM4(M4RotateQuat(rotation), result);
	result = M4MultiplyM4(M4ScaleV3(scale), result);
	result = M4MultiplyM4(M4TranslateV3(position), result);
	
	return result;
}

typedef struct String8
{
	u8 *str;
	u64 len;
}
String8;

internal String8
String8Init(u8 *str, u64 len)
{
	String8 s = {0};
	s.str = str;
	s.len = len;
	
	return s;
}

#define str8(s) String8Init((u8 *)(s), sizeof(s) - 1)

internal b32
String8Match(String8 a, String8 b)
{
	if(a.len != b.len)
	{
		return false;
	}
	
	for(i32 i = 0; i < a.len; i++)
	{
		if(a.str[i] != b.str[i])
		{
			return false;
		}
	}
	
	return true;
}

internal String8
String8BeforeFirstSubstringFromBackInclusive(String8 string, String8 substring)
{
	Assert("Substring cannot be larger than string." && string.len >= substring.len);
	
	String8 result = {0};
	result.str = string.str;
	result.len = 0;
	
	for(i32 i = string.len - substring.len - 1; i >= 0; i--)
	{
		String8 here = String8Init(string.str + i, substring.len);
		
		if(String8Match(here, substring))
		{
			result.len = i + substring.len;
			break;
		}
	}
	
	return result;
}

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
