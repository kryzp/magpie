#include "mat4.h"
#include "vec3.h"
#include "quat.h"
#include "maths.h"

Mat4::Mat4()
{
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			e[i][j] = 0.f;
		}
	}
}

Mat4::Mat4(float diagonal)
	: Mat4()
{
	m00 = diagonal;
	m11 = diagonal;
	m22 = diagonal;
	m33 = diagonal;
}

bool Mat4::operator==(const Mat4 &other) const
{
	for (int i = 0; i < 16; i++) {
		if ((&m00)[i] != (&other.m00)[i])
			return false;
	}

	return true;
}

Mat4 Mat4::Lerp(const Mat4 &to, float t) const
{
	Mat4 result;

	for (int i = 0; i < 16; i++)
		(&result.m00)[i] = maths::Lerp((&m00)[i], (&to.m00)[i], t);

	return result;
}

Mat4 Mat4::operator*(const Mat4 &other) const
{
	Mat4 c;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				c.e[i][j] += e[k][j] * other.e[i][k];
			}
		}
	}

	return c;
}

Vec4 Mat4::operator*(const Vec4 &v) const
{
	Vec4 result;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result[i] += e[j][i] * v[j];
		}
	}

	return result;
}

Vec3 Mat4::TransformPoint(const Vec3 &v) const
{
	Vec4 result = *this * Vec4(v, 1.f);

	return result.XYZ();
}

Mat4 Mat4::operator*(float f) const
{
	Mat4 result;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.e[i][j] = e[i][j] * f;
		}
	}

	return result;
}

Mat4 Mat4::LookAt(const Vec3 &eye, const Vec3 &centre, const Vec3 &up)
{
	Mat4 result;

	Vec3 yaxis = (centre - eye).Normalized();
	Vec3 xaxis = yaxis.Cross(up).Normalized();
	Vec3 zaxis = xaxis.Cross(yaxis);

	result.m00 = xaxis.x;
	result.m01 = xaxis.y;
	result.m02 = xaxis.z;
	result.m03 = -xaxis.Dot(eye);

	result.m10 = yaxis.x;
	result.m11 = yaxis.y;
	result.m12 = yaxis.z;
	result.m13 = -yaxis.Dot(eye);

	result.m20 = zaxis.x;
	result.m21 = zaxis.y;
	result.m22 = zaxis.z;
	result.m23 = -zaxis.Dot(eye);

	result.m30 = 0.f;
	result.m31 = 0.f;
	result.m32 = 0.f;
	result.m33 = 1.f;

	return result;
}

Mat4 Mat4::Perspective(float fov, float aspect, float near_plane, float far_plane)
{
	Mat4 result;

	float f = 1.f / std::tan(fov / 360.f * maths::PIf);

	result.m00 = f / aspect;
	result.m12 = f;
	result.m21 = far_plane / (far_plane - near_plane);
	result.m23 = -(far_plane * near_plane) / (far_plane - near_plane);
	result.m31 = 1.f;

	return result;
}

Mat4 Mat4::Orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane)
{
	Mat4 result;

	result.m00 = 2.f / (right - left);
	result.m12 = 2.f / (top - bottom);
	result.m21 = 1.f / (far_plane - near_plane);

	result.m03 = -(right + left) / (right - left);
	result.m13 = -(top + bottom) / (top - bottom);
	result.m23 = -near_plane / (far_plane - near_plane);
	result.m33 = 1.f;

	return result;
}

Mat4 Mat4::Orthographic2D(float left, float right, float bottom, float top, float near_plane, float far_plane)
{
	// the classic GL orthographic matrix, depth -1..1...
	Mat4 gl(1.f);
	gl.m00 = 2.f / (right - left);
	gl.m11 = 2.f / (top - bottom);
	gl.m22 = -2.f / (far_plane - near_plane);
	gl.m03 = -(right + left) / (right - left);
	gl.m13 = -(top + bottom) / (top - bottom);
	gl.m23 = -(far_plane + near_plane) / (far_plane - near_plane);

	// ...then depth remapped to 0..1
	Mat4 depth_remap(1.f);
	depth_remap.m22 = 0.5f;
	depth_remap.m23 = 0.5f;

	return depth_remap * gl;
}

Mat4 Mat4::Inverse() const
{
	const Mat4 &m = *this;

	float coef00 = (m.m22 * m.m33) - (m.m23 * m.m32);
	float coef02 = (m.m21 * m.m33) - (m.m23 * m.m31);
	float coef03 = (m.m21 * m.m32) - (m.m22 * m.m31);
	float coef04 = (m.m12 * m.m33) - (m.m13 * m.m32);
	float coef06 = (m.m11 * m.m33) - (m.m13 * m.m31);
	float coef07 = (m.m11 * m.m32) - (m.m12 * m.m31);
	float coef08 = (m.m12 * m.m23) - (m.m13 * m.m22);
	float coef10 = (m.m11 * m.m23) - (m.m13 * m.m21);
	float coef11 = (m.m11 * m.m22) - (m.m12 * m.m21);
	float coef12 = (m.m02 * m.m33) - (m.m03 * m.m32);
	float coef14 = (m.m01 * m.m33) - (m.m03 * m.m31);
	float coef15 = (m.m01 * m.m32) - (m.m02 * m.m31);
	float coef16 = (m.m02 * m.m23) - (m.m03 * m.m22);
	float coef18 = (m.m01 * m.m23) - (m.m03 * m.m21);
	float coef19 = (m.m01 * m.m22) - (m.m02 * m.m21);
	float coef20 = (m.m02 * m.m13) - (m.m03 * m.m12);
	float coef22 = (m.m01 * m.m13) - (m.m03 * m.m11);
	float coef23 = (m.m01 * m.m12) - (m.m02 * m.m11);

	Vec4 fac0 = Vec4(coef00, coef00, coef02, coef03);
	Vec4 fac1 = Vec4(coef04, coef04, coef06, coef07);
	Vec4 fac2 = Vec4(coef08, coef08, coef10, coef11);
	Vec4 fac3 = Vec4(coef12, coef12, coef14, coef15);
	Vec4 fac4 = Vec4(coef16, coef16, coef18, coef19);
	Vec4 fac5 = Vec4(coef20, coef20, coef22, coef23);

	Vec4 vec0 = Vec4(m.m01, m.m00, m.m00, m.m00);
	Vec4 vec1 = Vec4(m.m11, m.m10, m.m10, m.m10);
	Vec4 vec2 = Vec4(m.m21, m.m20, m.m20, m.m20);
	Vec4 vec3 = Vec4(m.m31, m.m30, m.m30, m.m30);

	Vec4 inv0 = (vec1 * fac0) - (vec2 * fac1) + (vec3 * fac2);
	Vec4 inv1 = (vec0 * fac0) - (vec2 * fac3) + (vec3 * fac4);
	Vec4 inv2 = (vec0 * fac1) - (vec1 * fac3) + (vec3 * fac5);
	Vec4 inv3 = (vec0 * fac2) - (vec1 * fac4) + (vec2 * fac5);

	const float sign_a[] = { +1.f, -1.f, +1.f, -1.f };
	const float sign_b[] = { -1.f, +1.f, -1.f, +1.f };

	Mat4 inverse;

	for (int i = 0; i < 4; i++) {
		inverse.e[0][i] = inv0[i] * sign_a[i];
		inverse.e[1][i] = inv1[i] * sign_b[i];
		inverse.e[2][i] = inv2[i] * sign_a[i];
		inverse.e[3][i] = inv3[i] * sign_b[i];
	}

	Vec4 row0 = Vec4(inverse.m00, inverse.m01, inverse.m02, inverse.m03);
	Vec4 m0 = Vec4(m.m00, m.m10, m.m20, m.m30);
	Vec4 dot0 = m0 * row0;
	float dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);

	float one_over_det = 1.f / dot1;

	return inverse * one_over_det;
}

Mat4 Mat4::Transposed() const
{
	Mat4 result;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.e[i][j] = e[j][i];
		}
	}

	return result;
}

Mat4 Mat4::Translate(const Vec3 &translation)
{
	Mat4 result = Identity();

	result.m03 = translation.x;
	result.m13 = translation.y;
	result.m23 = translation.z;
	result.m33 = 1.f;

	return result;
}

Mat4 Mat4::Scale(const Vec3 &scale)
{
	Mat4 result = Identity();

	result.m00 = scale.x;
	result.m11 = scale.y;
	result.m22 = scale.z;
	result.m33 = 1.f;

	return result;
}

Mat4 Mat4::RotateAxis(float angle, const Vec3 &axis)
{
	Mat4 result = Identity();

	float sin_theta = std::sin(angle);
	float cos_theta = std::cos(angle);
	float cos_inv = 1.f - cos_theta;

	result.m00 = (axis.x * axis.x * cos_inv) + cos_theta;
	result.m10 = (axis.x * axis.y * cos_inv) + (axis.z * sin_theta);
	result.m20 = (axis.x * axis.z * cos_inv) - (axis.y * sin_theta);

	result.m01 = (axis.y * axis.x * cos_inv) - (axis.z * sin_theta);
	result.m11 = (axis.y * axis.y * cos_inv) + cos_theta;
	result.m21 = (axis.y * axis.z * cos_inv) + (axis.x * sin_theta);

	result.m02 = (axis.z * axis.x * cos_inv) + (axis.y * sin_theta);
	result.m12 = (axis.z * axis.y * cos_inv) - (axis.x * sin_theta);
	result.m22 = (axis.z * axis.z * cos_inv) + cos_theta;

	return result;
}

Mat4 Mat4::RotateQuat(const Quat &q)
{
	Mat4 result = Identity();

	result.m00 = 1.f - 2.f * (q.y * q.y + q.z * q.z);
	result.m01 = 2.f * (q.x * q.y - q.z * q.w);
	result.m02 = 2.f * (q.x * q.z + q.y * q.w);

	result.m10 = 2.f * (q.x * q.y + q.z * q.w);
	result.m11 = 1.f - 2.f * (q.x * q.x + q.z * q.z);
	result.m12 = 2.f * (q.y * q.z - q.x * q.w);

	result.m20 = 2.f * (q.x * q.z - q.y * q.w);
	result.m21 = 2.f * (q.y * q.z + q.x * q.w);
	result.m22 = 1.f - 2.f * (q.x * q.x + q.y * q.y);

	return result;
}

Mat4 Mat4::Transform(const Vec3 &position, const Quat &rotation, const Vec3 &scale, const Vec3 &origin)
{
	Mat4 result = Identity();

	result = Translate(origin * -1.f) * result;
	result = RotateQuat(rotation) * result;
	result = Scale(scale) * result;
	result = Translate(position) * result;

	return result;
}

Mat4 Mat4::WithoutTranslation() const
{
	Mat4 m = *this;

	m.m03 = 0.f;
	m.m13 = 0.f;
	m.m23 = 0.f;
	m.m33 = 1.f;

	return m;
}

Mat4 Mat4::WithoutRotation() const
{
	Mat4 m = *this;

	Vec3 s = Vec3(
		Vec3(m.m00, m.m10, m.m20).Length(),
		Vec3(m.m01, m.m11, m.m21).Length(),
		Vec3(m.m02, m.m12, m.m22).Length()
	);

	m.m00 = s.x;
	m.m01 = 0.f;
	m.m02 = 0.f;

	m.m10 = 0.f;
	m.m11 = s.y;
	m.m12 = 0.f;

	m.m20 = 0.f;
	m.m21 = 0.f;
	m.m22 = s.z;

	return m;
}

Vec3 Mat4::GetTranslation() const
{
	return Vec3(m03, m13, m23);
}
