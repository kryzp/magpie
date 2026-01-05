#include "matrix.h"

#include "core/scratch.h"

#include "calc.h"

Mat4::Mat4()
	: m00(0.f), m10(0.f), m20(0.f), m30(0.f)
	, m01(0.f), m11(0.f), m21(0.f), m31(0.f)
	, m02(0.f), m12(0.f), m22(0.f), m32(0.f)
	, m03(0.f), m13(0.f), m23(0.f), m33(0.f)
{
}

Mat4::Mat4(float dia)
	: m00(dia), m10(0.f), m20(0.f), m30(0.f)
	, m01(0.f), m11(dia), m21(0.f), m31(0.f)
	, m02(0.f), m12(0.f), m22(dia), m32(0.f)
	, m03(0.f), m13(0.f), m23(0.f), m33(dia)
{
}

Mat4 Mat4::identity()
{
	return Mat4(1.f);
}

Mat4 Mat4::lookat(const Vec3 &eye, const Vec3 &centre, const Vec3 &up)
{
	Mat4 result = {};

	Vec3 yaxis = (centre - eye).normalized();
	Vec3 xaxis = Vec3::cross(yaxis, up).normalized();
	Vec3 zaxis = Vec3::cross(xaxis, yaxis);

	result.m00 = xaxis.x;
	result.m01 = xaxis.y;
	result.m02 = xaxis.z;
	result.m03 = -Vec3::dot(xaxis, eye);

	result.m10 = yaxis.x;
	result.m11 = yaxis.y;
	result.m12 = yaxis.z;
	result.m13 = -Vec3::dot(yaxis, eye);

	result.m20 = zaxis.x;
	result.m21 = zaxis.y;
	result.m22 = zaxis.z;
	result.m23 = -Vec3::dot(zaxis, eye);

	result.m30 = 0.f;
	result.m31 = 0.f;
	result.m32 = 0.f;
	result.m33 = 1.f;

	return result;
}

Mat4 Mat4::perspective(float fov, float aspect, float near, float far)
{
	Mat4 result = {};

	float f = CalcF::tan(fov / 360.f * CalcF::PI);

	result.m00 = f / aspect;
	result.m12 = f;
	result.m21 = (far + near) / (far - near);
	result.m23 = -(2.f * far * near) / (far - near);
	result.m31 = 1.f;

	return result;
}

Mat4 Mat4::orthographic(float left, float right, float bottom, float top, float near, float far)
{
	Mat4 result = {};

	result.m00 = 2.f / (right - left);
	result.m12 = 2.f / (top - bottom);
	result.m21 = 2.f / (far - near);

	result.m03 = -(right + left) / (right - left);
	result.m13 = -(top + bottom) / (top - bottom);
	result.m23 = -(far + near) / (far - near);
	result.m33 = 1.f;

	return result;
}

// Axis must be normalized.
Mat4 Mat4::rotate_axis(float angle, const Vec3 &axis)
{
	Mat4 result = identity();

	float sin_theta = CalcF::sin(angle);
	float cos_theta = CalcF::cos(angle);
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
Mat4 Mat4::rotate_quat(const Quat &q)
{
	Mat4 result = identity();

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

Mat4 Mat4::translate(const Vec3 &translation)
{
	Mat4 result = identity();

	result.m03 = translation.x;
	result.m13 = translation.y;
	result.m23 = translation.z;
	result.m33 = 1.f;

	return result;
}

Mat4 Mat4::scale(const Vec3 &scale)
{
	Mat4 result = identity();

	result.m00 = scale.x;
	result.m11 = scale.y;
	result.m22 = scale.z;
	result.m33 = 1.f;

	return result;
}

Mat4 Mat4::transform(
	const Vec3 &position,
	const Quat &rotation,
	const Vec3 &scale,
	const Vec3 &origin
)
{
	Mat4 result = identity();

	result = Mat4::translate(-origin) * result;
	result = Mat4::rotate_quat(rotation) * result;
	result = Mat4::scale(scale) * result;
	result = Mat4::translate(position) * result;

	return result;
}

Mat4 Mat4::remove_translation() const
{
	Mat4 result = *this;

	result.m03 = 0.f;
	result.m13 = 0.f;
	result.m23 = 0.f;
	result.m33 = 1.f;

	return result;
}

Mat4 Mat4::remove_rotation() const
{
	Mat4 result = *this;

	Vec3 scale(
		Vec3(m00, m10, m20).length(),
		Vec3(m01, m11, m21).length(),
		Vec3(m02, m12, m22).length()
	);

	result.m00 = scale.x;
	result.m01 = 0.f;
	result.m02 = 0.f;

	result.m10 = 0.f;
	result.m11 = scale.y;
	result.m12 = 0.f;

	result.m20 = 0.f;
	result.m21 = 0.f;
	result.m22 = scale.z;

	return result;
}

Mat4 Mat4::inverse() const
{
	float coef00 = m22 * m33 - m23 * m32;
	float coef02 = m21 * m33 - m23 * m31;
	float coef03 = m21 * m32 - m22 * m31;
	float coef04 = m12 * m33 - m13 * m32;
	float coef06 = m11 * m33 - m13 * m31;
	float coef07 = m11 * m32 - m12 * m31;
	float coef08 = m12 * m23 - m13 * m22;
	float coef10 = m11 * m23 - m13 * m21;
	float coef11 = m11 * m22 - m12 * m21;
	float coef12 = m02 * m33 - m03 * m32;
	float coef14 = m01 * m33 - m03 * m31;
	float coef15 = m01 * m32 - m02 * m31;
	float coef16 = m02 * m23 - m03 * m22;
	float coef18 = m01 * m23 - m03 * m21;
	float coef19 = m01 * m22 - m02 * m21;
	float coef20 = m02 * m13 - m03 * m12;
	float coef22 = m01 * m13 - m03 * m11;
	float coef23 = m01 * m12 - m02 * m11;

	float fac0[] = { coef00, coef00, coef02, coef03 };
	float fac1[] = { coef04, coef04, coef06, coef07 };
	float fac2[] = { coef08, coef08, coef10, coef11 };
	float fac3[] = { coef12, coef12, coef14, coef15 };
	float fac4[] = { coef16, coef16, coef18, coef19 };
	float fac5[] = { coef20, coef20, coef22, coef23 };

	float vec0[] = { m01, m00, m00, m00 };
	float vec1[] = { m11, m10, m10, m10 };
	float vec2[] = { m21, m20, m20, m20 };
	float vec3[] = { m31, m30, m30, m30 };

	ScratchArena scratch;

	auto v4_add = [&](float *a, float *b) -> float * {
		float *result = scratch.get_arena().push_array<float>(4);
		result[0] = a[0] + b[0];
		result[1] = a[1] + b[1];
		result[2] = a[2] + b[2];
		result[3] = a[3] + b[3];
		return result;
	};

	auto v4_sub = [&](float *a, float *b) -> float * {
		float *result = scratch.get_arena().push_array<float>(4);
		result[0] = a[0] - b[0];
		result[1] = a[1] - b[1];
		result[2] = a[2] - b[2];
		result[3] = a[3] - b[3];
		return result;
	};

	auto v4_mul = [&](float *a, float *b) -> float * {
		float *result = scratch.get_arena().push_array<float>(4);
		result[0] = a[0] * b[0];
		result[1] = a[1] * b[1];
		result[2] = a[2] * b[2];
		result[3] = a[3] * b[3];
		return result;
	};

	float *inv0 = v4_add(v4_sub(v4_mul(vec1, fac0), v4_mul(vec2, fac1)), v4_mul(vec3, fac2));
	float *inv1 = v4_add(v4_sub(v4_mul(vec0, fac0), v4_mul(vec2, fac3)), v4_mul(vec3, fac4));
	float *inv2 = v4_add(v4_sub(v4_mul(vec0, fac1), v4_mul(vec1, fac3)), v4_mul(vec3, fac5));
	float *inv3 = v4_add(v4_sub(v4_mul(vec0, fac2), v4_mul(vec1, fac4)), v4_mul(vec2, fac5));

	float sign_a[] = { +1.f, -1.f, +1.f, -1.f };
	float sign_b[] = { -1.f, +1.f, -1.f, +1.f };

	Mat4 inverse = {};

	for (int i = 0; i < 4; i++) {
		inverse.v[0][i] = inv0[i] * sign_a[i];
		inverse.v[1][i] = inv1[i] * sign_b[i];
		inverse.v[2][i] = inv2[i] * sign_a[i];
		inverse.v[3][i] = inv3[i] * sign_b[i];
	}

	float row0[] = { inverse.m00, inverse.m01, inverse.m02, inverse.m03 };
	float m0[] = { m00, m10, m20, m30 };
	float *dot0 = v4_mul(m0, row0);
	float dot1 = (dot0[0] + dot0[1]) + (dot0[2] + dot0[3]);

	float one_over_det = 1.f / dot1;

	return inverse * one_over_det;
}

Mat4 Mat4::transpose() const
{
	Mat4 result = {};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.v[i][j] = this->v[j][i];
		}
	}

	return result;
}

Mat4 Mat4::operator + (const Mat4 &other) const
{
	Mat4 c = {};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			c.v[i][j] = this->v[i][j] + other.v[i][j];
		}
	}

	return c;
}

Mat4 Mat4::operator - (const Mat4 &other) const
{
	Mat4 c = {};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			c.v[i][j] = this->v[i][j] - other.v[i][j];
		}
	}

	return c;
}

Mat4 Mat4::operator * (const Mat4 &other) const
{
	Mat4 c = {};

	for (int k = 0; k < 4; k++) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				c.v[k][i] += this->v[j][i] * other.v[k][j];
			}
		}
	}

	return c;
}

Vec3 Mat4::operator * (const Vec3 &vector) const
{
	float elements[] = { vector.x, vector.y, vector.z, 1.f };

	Vec3 v = {};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			v.v[i] += this->v[j][i] * elements[j];
		}
	}

	return v;
}

Mat4 Mat4::operator * (float scalar) const
{
	Mat4 r = {};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			r.v[i][j] = this->v[i][j] * scalar;
		}
	}

	return r;
}
