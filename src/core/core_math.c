#include "core_math.h"

float v2_dot(v2 a, v2 b)
{
	return (a.x * b.x + a.y * b.y);
}

float v2_length_squared(v2 v)
{
	return v2_dot(v, v);
}

float v2_length(v2 v)
{
	return square_root(v2_length_squared(v));
}

v2 v2_normalize(v2 v)
{
	float length = v2_length(v);

	v.x /= length;
	v.y /= length;

	return v;
}

v3 v3_add(v3 a, v3 b)
{
	return v3(a.x + b.x,
		  a.y + b.y,
		  a.z + b.z);
}

v3 v3_sub(v3 a, v3 b)
{
	return v3(a.x - b.x,
		  a.y - b.y,
		  a.z - b.z);
}

v3 v3_mul_float(v3 v, float f)
{
	v.x *= f;
	v.y *= f;
	v.z *= f;

	return v;
}

float v3_dot(v3 a, v3 b)
{
	float dot = (a.x * b.x + a.y * b.y + a.z * b.z);

	return dot;
}

v3 v3_cross(v3 a, v3 b)
{
	v3 result = v3(a.y * b.z - a.z * b.y,
		       a.z * b.x - a.x * b.z,
		       a.x * b.y - a.y * b.x);

	return result;
}

float v3_length_squared(v3 a)
{
	return v3_dot(a, a);
}

float v3_length(v3 a)
{
	return square_root(v3_length_squared(a));
}

v3 v3_normalize(v3 v)
{
	float length = v3_length(v);

	v3 result = {
		v.x / length,
		v.y / length,
		v.z / length,
	};

	return result;
}

float v3_max_value(v3 v)
{
	return max_value(v.x, max_value(v.y, v.z));
}

float v3_min_value(v3 v)
{
	return min_value(v.x, min_value(v.y, v.z));
}

bool v4_rect_has_point(v4 r, v2 p)
{
	return (p.x >= r.x &&
		p.x <= r.x + r.width &&
		p.y >= r.y &&
		p.y <= r.y + r.height);
}

float v4_dot(v4 a, v4 b)
{
	return (a.x * b.x +
		a.y * b.y +
		a.z * b.z +
		a.w * b.w);
}

v4 v4_add(v4 a, v4 b)
{
	return v4(a.x + b.x,
		  a.y + b.y,
		  a.z + b.z,
		  a.w + b.w);
}

v4 v4_sub(v4 a, v4 b)
{
	return v4(a.x - b.x,
		  a.y - b.y,
		  a.z - b.z,
		  a.w - b.w);
}

v4 v4_mul_float(v4 a, float f)
{
	return v4(a.x * f,
		  a.y * f,
		  a.z * f,
		  a.w * f);
}

v4 v4_mul_v4(v4 a, v4 b)
{
	return v4(a.x * b.x,
		  a.y * b.y,
		  a.z * b.z,
		  a.w * b.w);
}

float v4_length_squared(v4 v)
{
	return v4_dot(v, v);
}

float v4_length(v4 v)
{
	return square_root(v4_length_squared(v));
}

v4 quat_init_identity()
{
	return v4(0.f, 0.f, 0.f, 1.f);
}

v4 quat_init_axis(float angle, v3 axis)
{
	v4 q = {0};

	q.w = cosf(angle * .5f);
	q.x = sinf(angle * .5f) * axis.x;
	q.y = sinf(angle * .5f) * axis.y;
	q.z = sinf(angle * .5f) * axis.z;

	return q;
}

v4 quat_init_euler(float pitch, float yaw, float roll)
{
	float sr = sinf(roll  * .5f);
	float cr = cosf(roll  * .5f);
	float sp = sinf(pitch * .5f);
	float cp = cosf(pitch * .5f);
	float sy = sinf(yaw   * .5f);
	float cy = cosf(yaw   * .5f);

	return v4((sr * cp * cy) - (cr * sp * sy),
		  (cr * sp * cy) + (sr * cp * sy),
		  (cr * cp * sy) - (sr * sp * cy),
		  (cr * cp * cy) + (sr * sp * sy));
}

v3 quat_to_euler(v4 q)
{
	float t0 =            (2.f + ((q.w * q.x) + (q.y * q.z)));
	float t1 = 1.f      - (2.f * ((q.x * q.x) + (q.y * q.y)));
	float t2 = clamp_value(2.f * ((q.w * q.y) - (q.z * q.x)), -1.f, 1.f);
	float t3 =            (2.f * ((q.w * q.z) + (q.x * q.y)));
	float t4 = 1.f      - (2.f * ((q.y * q.y) + (q.z * q.z)));

	float pitch = asinf(t2);
	float yaw   = atan2f(t3, t4);
	float roll  = atan2f(t0, t1);

	return v3(pitch, yaw, roll);
}

v4 quat_inverse(v4 q)
{
	v4 inverse = {0};

	float prod = v4_length_squared(q);

	if (prod > EPSILONf) {
		inverse.x = -q.x / square_root(prod);
		inverse.x = -q.y / square_root(prod);
		inverse.x = -q.z / square_root(prod);
		inverse.w =  q.w / square_root(prod);
	} else {
		inverse.x = -q.x;
		inverse.y = -q.y;
		inverse.z = -q.z;
		inverse.w =  q.w;
	}

	return inverse;
}

m4 m4_mul_m4(m4 a, m4 b)
{
	m4 c = {0};

	for (int k = 0; k < 4; k++) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				m4e(c, i, k) += m4e(a, i, j) * m4e(b, j, k);
			}
		}
	}

	return c;
}

v4 m4_mul_v4(m4 m, v4 v)
{
	v4 r = {0};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			r.v[i] += m4e(m, i, j) * v.v[j];
		}
	}

	return r;
}

v3 m4_mul_v3(m4 m, v3 v)
{
	v4 w = { v.x, v.y, v.z, 1.f };
	return m4_mul_v4(m, w).xyz;
}

m4 m4_mul_float(m4 m, float f)
{
	m4 r = {0};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			m4e(r, j, i) = m4e(m, j, i) * f;
		}
	}

	return r;
}

m4 m4_transpose(m4 m)
{
	m4 result = {0};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			m4e(result, j, i) = m4e(m, i, j);
		}
	}

	return result;
}

m4 m4_translate(v3 translation)
{
	m4 result = m4(1.f);

	result.m03 = translation.x;
	result.m13 = translation.y;
	result.m23 = translation.z;
	result.m33 = 1.f;

	return result;
}

m4 m4_scale(v3 scale)
{
	m4 result = m4(1.f);

	result.m00 = scale.x;
	result.m11 = scale.y;
	result.m22 = scale.z;
	result.m33 = 1.f;

	return result;
}

m4 m4_lookat(v3 eye, v3 center, v3 up)
{
	m4 result = {0};

	v3 yaxis = v3_normalize(v3_sub(center, eye));
	v3 xaxis = v3_normalize(v3_cross(yaxis, up));
	v3 zaxis = v3_cross(xaxis, yaxis);

	result.m00 = xaxis.x;
	result.m01 = xaxis.y;
	result.m02 = xaxis.z;
	result.m03 = -v3_dot(xaxis, eye);

	result.m10 = yaxis.x;
	result.m11 = yaxis.y;
	result.m12 = yaxis.z;
	result.m13 = -v3_dot(yaxis, eye);

	result.m20 = zaxis.x;
	result.m21 = zaxis.y;
	result.m22 = zaxis.z;
	result.m23 = -v3_dot(zaxis, eye);

	result.m30 = 0.f;
	result.m31 = 0.f;
	result.m32 = 0.f;
	result.m33 = 1.f;

	return result;
}

m4 m4_perspective(float fov, float aspect,
			   float near, float far)
{
	m4 result = {0};

	float f = tanf(fov / 360.f * PIf);

	result.m00 = f / aspect;
	result.m12 = f;
	result.m21 = (far + near) / (far - near);
	result.m23 = -(2.f * far * near) / (far - near);
	result.m31 = 1.f;

	return result;
}

m4 m4_orthographic(float left, float right,
			    float bottom, float top,
			    float near, float far)
{
	m4 result = {0};

	result.m00 = 2.f / (right - left);
	result.m12 = 2.f / (top - bottom);
	result.m21 = 2.f / (far - near);

	result.m03 = -(right + left) / (right - left);
	result.m13 = -(top + bottom) / (top - bottom);
	result.m23 = -(far + near) / (far - near);
	result.m33 = 1.f;

	return result;
}

m4 m4_inverse(m4 m)
{
	float coef00 = m.m22 * m.m33 - m.m23 * m.m32;
	float coef02 = m.m21 * m.m33 - m.m23 * m.m31;
	float coef03 = m.m21 * m.m32 - m.m22 * m.m31;
	float coef04 = m.m12 * m.m33 - m.m13 * m.m32;
	float coef06 = m.m11 * m.m33 - m.m13 * m.m31;
	float coef07 = m.m11 * m.m32 - m.m12 * m.m31;
	float coef08 = m.m12 * m.m23 - m.m13 * m.m22;
	float coef10 = m.m11 * m.m23 - m.m13 * m.m21;
	float coef11 = m.m11 * m.m22 - m.m12 * m.m21;
	float coef12 = m.m02 * m.m33 - m.m03 * m.m32;
	float coef14 = m.m01 * m.m33 - m.m03 * m.m31;
	float coef15 = m.m01 * m.m32 - m.m02 * m.m31;
	float coef16 = m.m02 * m.m23 - m.m03 * m.m22;
	float coef18 = m.m01 * m.m23 - m.m03 * m.m21;
	float coef19 = m.m01 * m.m22 - m.m02 * m.m21;
	float coef20 = m.m02 * m.m13 - m.m03 * m.m12;
	float coef22 = m.m01 * m.m13 - m.m03 * m.m11;
	float coef23 = m.m01 * m.m12 - m.m02 * m.m11;

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

	v4 inv0 = v4_add(v4_sub(v4_mul_v4(vec1, fac0), v4_mul_v4(vec2, fac1)), v4_mul_v4(vec3, fac2));
	v4 inv1 = v4_add(v4_sub(v4_mul_v4(vec0, fac0), v4_mul_v4(vec2, fac3)), v4_mul_v4(vec3, fac4));
	v4 inv2 = v4_add(v4_sub(v4_mul_v4(vec0, fac1), v4_mul_v4(vec1, fac3)), v4_mul_v4(vec3, fac5));
	v4 inv3 = v4_add(v4_sub(v4_mul_v4(vec0, fac2), v4_mul_v4(vec1, fac4)), v4_mul_v4(vec2, fac5));

	float sign_a[] = { +1.f, -1.f, +1.f, -1.f };
	float sign_b[] = { -1.f, +1.f, -1.f, +1.f };

	m4 inverse = {0};
	
	for (int i = 0; i < 4; i++) {
		m4e(inverse, i, 0) = inv0.v[i] * sign_a[i];
		m4e(inverse, i, 1) = inv1.v[i] * sign_b[i];
		m4e(inverse, i, 2) = inv2.v[i] * sign_a[i];
		m4e(inverse, i, 3) = inv3.v[i] * sign_b[i];
	}

	v4 row0 = { inverse.m00, inverse.m01, inverse.m02, inverse.m03 };
	v4 m0 = { m.m00, m.m10, m.m20, m.m30 };
	v4 dot0 = v4_mul_v4(m0, row0);
	float dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);

	float one_over_det = 1.f / dot1;

	return m4_mul_float(inverse, one_over_det);
}

m4 m4_remove_translation(m4 m)
{
	m.m03 = 0.f;
	m.m13 = 0.f;
	m.m23 = 0.f;
	m.m33 = 1.f;

	return m;
}

m4 m4_remove_rotation(m4 m)
{
	v3 scale = {
		v3_length(v3(m.m00, m.m10, m.m20)),
		v3_length(v3(m.m01, m.m11, m.m21)),
		v3_length(v3(m.m02, m.m12, m.m22)),
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

m4 m4_rotate_axis(float angle, v3 axis)
{
	m4 result = m4(1.f);

	axis = v3_normalize(axis);

	float sin_theta = sinf(angle);
	float cos_theta = cosf(angle);
	float cos_inv   = 1.f - cos_theta;

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

// Input quaternion must be normalized.
m4 m4_rotate_quat(v4 q)
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

m4 m4_transform(v3 position, v4 rotation, v3 scale, v3 origin)
{
	m4 result = m4(1.f);

	result = m4_mul_m4(m4_translate(v3_mul_float(origin, -1.f)), result);
	result = m4_mul_m4(m4_rotate_quat(rotation), result);
	result = m4_mul_m4(m4_scale(scale), result);
	result = m4_mul_m4(m4_translate(position), result);

	return result;
}

v3 spherical_to_cartesian(float r, float phi, float theta)
{
	return v3(r * cosf(theta) * cosf(phi),
		  r * cosf(theta) * sinf(phi),
		  r * sinf(theta));
}
