#ifndef CORE_MATH_H
#define CORE_MATH_H

#include <math.h>
#include <float.h>

#define MATH_PI                                  3.1415926535897
#define MATH_PIf                                 3.1415926535897f
#define MATH_ONE_OVER_SQUARE_ROOT_OF_TWO_PI      0.3989422804
#define MATH_ONE_OVER_SQUARE_ROOT_OF_TWO_PIf     0.3989422804f
#define MATH_EULERS_NUMBER                       2.718281828459045
#define MATH_EULERS_NUMBERf                      2.718281828459045f
#define MATH_EPSILON DBL_EPSILON
#define MATH_EPSILONf FLT_EPSILON

#define SquareRoot   sqrtf
#define AbsF         fabsf
#define CosF         cosf
#define SinF         sinf
#define TanF         tanf
#define ASinF        asinf
#define ACosF        acosf
#define ATanF        atanf
#define ATan2F       atan2f
#define LogF         logf
#define Log2F        log2f
#define Log10F       log10f

#define MinValue(a, b) (((a) < (b)) ? (a) : (b))
#define MaxValue(a, b) (((a) > (b)) ? (a) : (b))
#define ClampValue(v, lo, hi) MaxValue((lo), MinValue((hi), (v)))
#define LerpValue(from, to, t) ((from) + ((to) - (from)) * (t))
#define ApproachValue(from, to, t) (((to) > (from)) ? MinValue(((from) + (amount)), (to)) : MaxValue(((from) - (amount)), (to)))

typedef union v2 v2;
union v2
{
	struct
	{
		f32 x;
		f32 y;
	};

	f32 v[2];
};

#define v2(x_, y_)  ((v2) { .x = (x_), .y = (y_) })
#define v2x(x_)     ((v2) { .x = (x_), .y = (x_) })

internal f32 V2Dot(v2 a, v2 b);

internal f32 V2LengthSqr(v2 v);
internal f32 V2Length(v2 v);

typedef union v3 v3;
union v3
{
	struct
	{
		f32 x;
		f32 y;
		f32 z;
	};

	f32 v[3];
};

#define v3(x_, y_, z_)  ((v3) { .x = (x_), .y = (y_), .z = (z_) })
#define v3x(x_)         ((v3) { .x = (x_), .y = (x_), .z = (x_) })

internal v3 V3Add(v3 a, v3 b);
internal v3 V3Sub(v3 a, v3 b);

internal v3 V3MulF32(v3 v, f32 f);
internal v3 V3MulV3(v3 a, v3 b);

internal f32 V3Dot(v3 a, v3 b);
internal v3 V3Cross(v3 a, v3 b);

internal f32 V3LengthSqr(v3 v);
internal f32 V3Length(v3 v);

internal v3 V3Normalize(v3 v);

internal f32 V3Max(v3 v);
internal f32 V3Min(v3 v);

internal v3 V3Lerp(v3 from, v3 to, f32 amount);
internal v3 V3Approach(v3 from, v3 to, f32 amount);

internal v3 V3Reflect(v3 v, v3 n);
internal v3 V3Refract(v3 v, v3 n, f64 eta21);

internal v3 V3SphericalToCartesian(f32 radius, f32 azimuth, f32 elevation);

typedef union v4 v4;
union v4
{
	struct
	{
		f32 x;
		f32 y;

		union
		{
			struct
			{
				float z;
				float w;
			};

			struct
			{
				float width;
				float height;
			};
		};
	};

	f32 v[4];
};

#define v4(x_, y_, z_, w_)  ((v4) { .x = (x_), .y = (y_), .z = (z_), .w = (w_) })
#define v4x(x_)             ((v4) { .x = (x_), .y = (x_), .z = (x_), .w = (x_) })

internal v4 V4Add(v4 a, v4 b);
internal v4 V4Sub(v4 a, v4 b);

internal v4 V4MulF32(v4 v, f32 f);
internal v4 V4MulV4(v4 a, v4 b);

internal f32 V4Dot(v4 a, v4 b);

internal f32 V4LengthSqr(v4 v);
internal f32 V4Length(v4 v);

internal v4 V4FrustumNormalizePlane(v4 v);

#define V4QuatIdentity() v4(0.f, 0.f, 0.f, 1.f)

internal v4 V4QuatInitAxis(f32 angle, v3 axis);
internal v4 V4QuatInitEuler(v3 euler); // Pitch, Yaw, Roll

internal v3 V4QuatToEuler(v4 q); // Pitch, Yaw, Roll
internal v4 V4QuatInverse(v4 q);

typedef union m4 m4;
union m4
{
	struct
	{
		f32 m00, m10, m20, m30;
		f32 m01, m11, m21, m31;
		f32 m02, m12, m22, m32;
		f32 m03, m13, m23, m33;
	};

	v4 c[4];
	f32 e[4][4];
};

#define m4(d) ((m4) {							\
			(d), 0.f, 0.f, 0.f,					\
			0.f, (d), 0.f, 0.f,					\
			0.f, 0.f, (d), 0.f,					\
			0.f, 0.f, 0.f, (d)					\
		})

#define M4Identity() m4(1.f)

internal m4 M4MulM4(m4 a, m4 b);
internal v4 M4MulV4(m4 m, v4 v);
internal v3 M4MulV3(m4 m, v3 v);
internal m4 M4MulF32(m4 m, f32 f);

internal m4 M4LookAt(v3 eye, v3 centre, v3 up);

internal m4 M4Perspective(f32 fov, f32 aspect, f32 near, f32 far);
internal m4 M4Orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);

internal m4 M4Inverse(m4 m);
internal m4 M4Transpose(m4 m);

internal m4 M4Translate(v3 translation);
internal m4 M4Scale(v3 scale);

internal m4 M4RotateAround(f32 angle, v3 normal); // Must be normalized.
internal m4 M4RotateAxis(f32 angle, v3 axis); // Must be normalized.
internal m4 M4RotateQuat(v4 q); // Must be normalized.

internal m4 M4Transform(v3 position, v4 rotation, v3 scale, v3 origin);

internal m4 M4RemoveTranslation(m4 m);
internal m4 M4RemoveRotation(m4 m);

#endif // CORE_MATH_H
