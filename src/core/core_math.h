#ifndef CORE_MATH_H
#define CORE_MATH_H

/*
 * v2, v3, v4, m4 all use typedef because
 * of how commonly they're used. Treated like
 * being "on-par" with int, float, etc...
 */

#include "core_types.h"

typedef struct v2 {
	float x;
	float y;
} v2;

static inline v2 v2_init(float x, float y)
{
	v2 v = { x, y };
	return v;
}

#define v2(x, y) v2_init(x, y)
#define v2x(x) v2_init(x, x)

float v2_dot(v2 a, v2 b);

float v2_length_squared(v2 v);
float v2_length(v2 v);

v2 v2_normalize(v2 v);

typedef union v3 {
	struct {
		float x;
		float y;
		float z;
	};

	struct {
		float r;
		float g;
		float b;
	};

	float v[3];
} v3;

static inline v3 v3_init(float x, float y, float z)
{
	v3 v = { x, y, z };
	return v;
}

#define v3(x, y, z) v3_init(x, y, z)
#define v3x(x) v3_init(x, x, x)

v3 v3_add(v3 a, v3 b);
v3 v3_sub(v3 a, v3 b);

v3 v3_mul_float(v3 v, float f);

float v3_dot(v3 a, v3 b);
v3 v3_cross(v3 a, v3 b);

float v3_length_squared(v3 a);
float v3_length(v3 a);

v3 v3_normalize(v3 v);

float v3_max_value(v3 v);
float v3_min_value(v3 v);

typedef union v4 {
	struct {
		float x;
		float y;

		union {
			struct {
				float z;

				union {
					float w;
					float radius;
				};
			};

			struct {
				float width;
				float height;
			};
		};
	};

	struct {
		v3 xyz;
	};

	struct {
		float r;
		float g;
		float b;
		float a;
	};

	float v[4];
} v4;

static inline v4 v4_init(float x, float y, float z, float w)
{
	v4 v = { x, y, z, w };
	return v;
}

#define v4(x, y, z, w) v4_init(x, y, z, w)
#define v4x(x) v4_init(x, x, x, x)

bool v4_rect_has_point(v4 r, v2 p);

float v4_dot(v4 a, v4 b);

v4 v4_add(v4 a, v4 b);
v4 v4_sub(v4 a, v4 b);

v4 v4_mul_float(v4 a, float f);
v4 v4_mul_v4(v4 a, v4 b);

float v4_length_squared(v4 v);
float v4_length(v4 v);

v4 quat_init_identity();
v4 quat_init_axis(float angle, v3 axis);
v4 quat_init_euler(float pitch, float yaw, float roll);
v3 quat_to_euler(v4 q);
v4 quat_inverse(v4 q);

typedef struct v2i {
	s32 x;
	s32 y;
} v2i;

static inline v2i v2i_init(s32 x, s32 y)
{
	v2i v = { x, y };
	return v;
}

#define v2i(x, y) v2i_init(x, y)

typedef union v3i {
	struct {
		s32 x;
		s32 y;
		s32 z;
	};

	struct {
		s32 r;
		s32 g;
		s32 b;
	};

	s32 v[3];
} v3i;

static inline v3i v3i_init(s32 x, s32 y, s32 z)
{
	v3i v = { x, y, z };
	return v;
}

#define v3i(x, y, z) v3i_init(x, y, z)

typedef union v4i {
	struct {
		s32 x;
		s32 y;

		union {
			struct {
				s32 z;
				s32 w;
			};

			struct {
				s32 width;
				s32 height;
			};
		};
	};

	struct {
		s32 r;
		s32 g;
		s32 b;
		s32 a;
	};

	s32 v[4];
} v4i;

static inline v4i v4i_init(s32 x, s32 y, s32 z, s32 w)
{
	v4i v = { x, y, z, w };
	return v;
}

#define v4i(x, y, z, w) v4i_init(x, y, z, w)

// Column-major layout.
typedef union m4 {
	struct {
		float m00, m10, m20, m30;
		float m01, m11, m21, m31;
		float m02, m12, m22, m32;
		float m03, m13, m23, m33;
	};

	v4 c[4];
	float e[4][4];
} m4;

static inline m4 m4_init(float dia)
{
	m4 m = {
		dia, 0.f, 0.f, 0.f,
		0.f, dia, 0.f, 0.f,
		0.f, 0.f, dia, 0.f,
		0.f, 0.f, 0.f, dia
	};

	return m;
}

#define m4(d) m4_init(d)
#define m4e(matrix, row_index, col_index) ((matrix).e[(col_index)][(row_index)])

m4 m4_mul_m4(m4 a, m4 b);
v4 m4_mul_v4(m4 m, v4 v);
v3 m4_mul_v3(m4 m, v3 v);
m4 m4_mul_float(m4 m, float f);

m4 m4_lookat(v3 eye, v3 center, v3 up);

m4 m4_perspective(float fov, float aspect,
		  float near, float far);

m4 m4_orthographic(float left, float right,
		   float bottom, float top,
		   float near, float far);

m4 m4_inverse(m4 m);

m4 m4_transpose(m4 m);
m4 m4_translate(v3 translation);
m4 m4_scale(v3 scale);

m4 m4_remove_translation(m4 m);
m4 m4_remove_rotation(m4 m);

m4 m4_rotate_axis(float angle, v3 axis);
m4 m4_rotate_quat(v4 q); // Input quaternion must be normalized.

m4 m4_transform(v3 position, v4 rotation, v3 scale, v3 origin);

// ---

v3 spherical_to_cartesian(float r, float phi, float theta);

#endif // CORE_MATH_H
