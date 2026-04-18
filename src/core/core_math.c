
internal f32
V2Dot(v2 a, v2 b)
{
	return (a.x * a.x +
			a.y * b.y);
}

internal f32
V2LengthSqr(v2 v)
{
	return V2Dot(v, v);
}

internal f32
V2Length(v2 v)
{
	return SquareRoot(V2LengthSqr(v));
}

internal v3
V3Add(v3 a, v3 b)
{
	return v3(a.x + b.x,
			  a.y + b.y,
			  a.z + b.z);
}

internal v3
V3Sub(v3 a, v3 b)
{
	return v3(a.x - b.x,
			  a.y - b.y,
			  a.z - b.z);
}

internal v3
V3MulF32(v3 v, f32 f)
{
	return v3(v.x * f,
			  v.y * f,
			  v.z * f);
}

internal v3
V3MulV3(v3 a, v3 b)
{
	return v3(a.x * b.x,
			  a.y * b.y,
			  a.z * b.z);
}

internal f32
V3Dot(v3 a, v3 b)
{
	return (a.x * b.x +
			a.y * b.y +
			a.z * b.z);
}

internal v3
V3Cross(v3 a, v3 b)
{
	return v3((a.y * b.z) - (a.z * b.y),
			  (a.z * b.x) - (a.x * b.z),
			  (a.x * b.y) - (a.y * b.x));
}

internal f32
V3LengthSqr(v3 v)
{
	return V3Dot(v, v);
}

internal f32
V3Length(v3 v)
{
	return SquareRoot(V3LengthSqr(v));
}

internal v3
V3Normalize(v3 v)
{
	return V3MulF32(v, 1.f / V3Length(v));
}

internal f32
V3Max(v3 v)
{
	return MaxValue(v.x, MaxValue(v.y, v.z));
}

internal f32
V3Min(v3 v)
{
	return MinValue(v.x, MinValue(v.y, v.z));
}

internal v3
V3MinOf(v3 a, v3 b)
{
	return v3(MinValue(a.x, b.x),
			  MinValue(a.y, b.y),
			  MinValue(a.z, b.z));
}

internal v3
V3MaxOf(v3 a, v3 b)
{
	return v3(MaxValue(a.x, b.x),
			  MaxValue(a.y, b.y),
			  MaxValue(a.z, b.z));
}

internal v3
V3Lerp(v3 from, v3 to, f32 amount)
{
	return v3(LerpValue(from.x, to.x, amount),
			  LerpValue(from.y, to.y, amount),
			  LerpValue(from.z, to.z, amount));
}

internal v3
V3Approach(v3 from, v3 to, f32 amount)
{
	return v3(ApproachValue(from.x, to.x, amount),
			  ApproachValue(from.y, to.y, amount),
			  ApproachValue(from.z, to.z, amount));
}

internal v3
V3Reflect(v3 v, v3 n)
{
	// reflected = v - 2(v.n)n
	return V3Sub(v, V3MulF32(n, 2.f * V3Dot(v, n)));
}

internal v3
V3Refract(v3 v, v3 n, f64 eta21)
{
	f64 cost = MinValue(-V3Dot(v, n), 1.f);
	v3 out_perp = V3MulF32(V3Add(v, V3MulF32(n, cost)), eta21); // eta . (v + cost . n)
	v3 out_para = V3MulF32(n, -SquareRoot(AbsF(1.f - V3LengthSqr(out_perp))));
	return V3Add(out_perp, out_para);
}

internal v3
V3SphericalToCartesian(f32 radius, f32 azimuth, f32 elevation)
{
	return v3(radius * CosF(elevation) * CosF(azimuth),
			  radius * CosF(elevation) * SinF(azimuth),
			  radius * SinF(elevation));
}

internal v4
V4Add(v4 a, v4 b)
{
	return v4(a.x + b.x,
			  a.y + b.y,
			  a.z + b.z,
			  a.w + b.w);
}

internal v4
V4Sub(v4 a, v4 b)
{
	return v4(a.x - b.x,
			  a.y - b.y,
			  a.z - b.z,
			  a.w - b.w);
}

internal v4
V4MulF32(v4 v, f32 f)
{
	return v4(v.x * f,
			  v.y * f,
			  v.z * f,
			  v.w * f);
}

internal v4
V4MulV4(v4 a, v4 b)
{
	return v4(a.x * b.x,
			  a.y * b.y,
			  a.z * b.z,
			  a.w * b.w);
}

internal f32
V4Dot(v4 a, v4 b)
{
	return (a.x * b.x +
			a.y * b.y +
			a.z * b.z +
			a.w * b.w);
}

internal f32
V4LengthSqr(v4 v)
{
	return V4Dot(v, v);
}

internal f32
V4Length(v4 v)
{
	return SquareRoot(V4LengthSqr(v));
}

internal v4
V4FrustumNormalizePlane(v4 v)
{
	f32 length = SquareRoot(v.x * v.x +
							v.y * v.y +
							v.z * v.z);
	
	return V4MulF32(v, 1.f / length);
}

internal v4
V4QuatInitAxis(f32 angle, v3 axis)
{
	v4 q = {0};

	q.w = CosF(angle * .5f);
	q.x = SinF(angle * .5f) * axis.z;
	q.y = SinF(angle * .5f) * axis.y;
	q.z = SinF(angle * .5f) * axis.z;

	return q;
}

internal v4
V4QuatInitEuler(v3 euler)
{
	// Pitch
	f32 sp = SinF(euler.x * .5f);
	f32 cp = CosF(euler.x * .5f);

	// Yaw
	f32 sy = SinF(euler.y * .5f);
	f32 cy = CosF(euler.y * .5f);

	// Roll
	f32 sr = SinF(euler.z * .5f);
	f32 cr = CosF(euler.z * .5f);
	
	return v4((sr * cp * cy) - (cr * sp * sy),
			  (cr * sp * cy) + (sr * cp * sy),
			  (cr * cp * sy) - (sr * sp * cy),
			  (cr * cp * cy) + (sr * sp * sy));
}

internal v3
V4QuatToEuler(v4 q)
{
	f32 t0 =           (2.f + ((q.w * q.x) + (q.y * q.z)));
	f32 t1 = 1.f     - (2.f * ((q.x * q.x) + (q.y * q.y)));
	f32 t2 = ClampValue(2.f * ((q.w * q.y) - (q.z * q.x)), -1.f, 1.f);
	f32 t3 =           (2.f * ((q.w * q.z) + (q.x * q.y)));
	f32 t4 = 1.f     - (2.f * ((q.y * q.y) + (q.z * q.z)));

	f32 p = ASinF(t2);
	f32 y = ATan2F(t3, t4);
	f32 r = ATan2F(t0, t1);

	return v3(p, y, r);
}

internal v4
V4QuatInverse(v4 q)
{
	v4 inverse = {0};

	f32 length_sqr = V4LengthSqr(q);

	if (length_sqr > MATH_EPSILON_F32)
	{
		f32 length = SquareRoot(length_sqr);

		inverse.x = -q.x / length;
		inverse.y = -q.y / length;
		inverse.z = -q.z / length;
		inverse.w =  q.w / length;
	}
	else
	{
		inverse.x = -q.x;
		inverse.y = -q.y;
		inverse.z = -q.z;
		inverse.w =  q.w;
	}

	return inverse;
}

internal m4
M4MulM4(m4 a, m4 b)
{
	m4 c = {0};

	for (u32 k = 0; k < 4; k++)
	{
		for (u32 i = 0; i < 4; i++)
		{
			for (u32 j = 0; j < 4; j++)
			{
				c.e[k][i] += a.e[j][i] * b.e[i][j];
			}
		}
	}

	return c;
}

internal v4
M4MulV4(m4 m, v4 v)
{
	v4 result = {0};

	for (u32 i = 0; i < 4; i++)
	{
		for (u32 j = 0; j < 4; j++)
		{
			result.v[i] += m.e[j][i] * v.v[j];
		}
	}

	return result;
}

internal v3
M4MulV3(m4 m, v3 v)
{
	v4 result = M4MulV4(m, v4(v.x, v.y, v.z, 1.f));

	return v3(result.x, result.y, result.z);
}

internal m4
M4MulF32(m4 m, f32 f)
{
	m4 result = {0};

	for (u32 i = 0; i < 4; i++)
	{
		for (u32 j = 0; j < 4; j++)
		{
			result.e[i][j] = m.e[i][j] * f;
		}
	}

	return result;
}

internal m4
M4LookAt(v3 eye, v3 centre, v3 up)
{
	m4 result = {0};

	v3 yaxis = V3Normalize(V3Sub(centre, eye));
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
M4Perspective(f32 fov, f32 aspect, f32 near, f32 far)
{
	m4 result = {0};

	f32 f = 1.f / TanF(fov / 360.f * MATH_PI);

	result.m00 = f / aspect;
	result.m12 = f;
	result.m21 = (far + near) / (far - near);
	result.m23 = -(2.f * far * near) / (far - near);
	result.m31 = 1.f;

	return result;
}

internal m4
M4Orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
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

internal m4
M4Inverse(m4 m)
{
	f32 coef00 = (m.m22 * m.m33) - (m.m23 * m.m32);
	f32 coef02 = (m.m21 * m.m33) - (m.m23 * m.m31);
	f32 coef03 = (m.m21 * m.m32) - (m.m22 * m.m31);
	f32 coef04 = (m.m12 * m.m33) - (m.m13 * m.m32);
	f32 coef06 = (m.m11 * m.m33) - (m.m13 * m.m31);
	f32 coef07 = (m.m11 * m.m32) - (m.m12 * m.m31);
	f32 coef08 = (m.m12 * m.m23) - (m.m13 * m.m22);
	f32 coef10 = (m.m11 * m.m23) - (m.m13 * m.m21);
	f32 coef11 = (m.m11 * m.m22) - (m.m12 * m.m21);
	f32 coef12 = (m.m02 * m.m33) - (m.m03 * m.m32);
	f32 coef14 = (m.m01 * m.m33) - (m.m03 * m.m31);
	f32 coef15 = (m.m01 * m.m32) - (m.m02 * m.m31);
	f32 coef16 = (m.m02 * m.m23) - (m.m03 * m.m22);
	f32 coef18 = (m.m01 * m.m23) - (m.m03 * m.m21);
	f32 coef19 = (m.m01 * m.m22) - (m.m02 * m.m21);
	f32 coef20 = (m.m02 * m.m13) - (m.m03 * m.m12);
	f32 coef22 = (m.m01 * m.m13) - (m.m03 * m.m11);
	f32 coef23 = (m.m01 * m.m12) - (m.m02 * m.m11);

	v4 fac0 = v4(coef00, coef00, coef02, coef03);
	v4 fac1 = v4(coef04, coef04, coef06, coef07);
	v4 fac2 = v4(coef08, coef08, coef10, coef11);
	v4 fac3 = v4(coef12, coef12, coef14, coef15);
	v4 fac4 = v4(coef16, coef16, coef18, coef19);
	v4 fac5 = v4(coef20, coef20, coef22, coef23);

	v4 vec0 = v4(m.m01, m.m00, m.m00, m.m00);
	v4 vec1 = v4(m.m11, m.m10, m.m10, m.m10);
	v4 vec2 = v4(m.m21, m.m20, m.m20, m.m20);
	v4 vec3 = v4(m.m31, m.m30, m.m30, m.m30);

	v4 inv0 = V4Add(V4Sub(V4MulV4(vec1, fac0), V4MulV4(vec2, fac1)), V4MulV4(vec3, fac2));
	v4 inv1 = V4Add(V4Sub(V4MulV4(vec0, fac0), V4MulV4(vec2, fac3)), V4MulV4(vec3, fac4));
	v4 inv2 = V4Add(V4Sub(V4MulV4(vec0, fac1), V4MulV4(vec1, fac3)), V4MulV4(vec3, fac5));
	v4 inv3 = V4Add(V4Sub(V4MulV4(vec0, fac2), V4MulV4(vec1, fac4)), V4MulV4(vec2, fac5));

	f32 sign_a[] = { +1.f, -1.f, +1.f, -1.f };
	f32 sign_b[] = { -1.f, +1.f, -1.f, +1.f };

	m4 inverse = {0};

	for (u32 i = 0; i < 4; i++)
	{
		inverse.e[0][i] = inv0.v[i] * sign_a[i];
		inverse.e[1][i] = inv1.v[i] * sign_b[i];
		inverse.e[2][i] = inv2.v[i] * sign_a[i];
		inverse.e[3][i] = inv3.v[i] * sign_b[i];
	}

	v4 row0 = v4(inverse.m00, inverse.m01, inverse.m02, inverse.m03);
	v4 m0 = v4(m.m00, m.m10, m.m20, m.m30);
	v4 dot0 = V4MulV4(m0, row0);
	f32 dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);

	f32 one_over_det = 1.f / dot1;

	return M4MulF32(inverse, one_over_det);
}

internal m4
M4Transpose(m4 m)
{
	m4 result = {0};

	for (u32 i = 0; i < 4; i++)
	{
		for (u32 j = 0; j < 4; j++)
		{
			result.e[i][j] = m.e[j][i];
		}
	}

	return result;
}

internal m4
M4Translate(v3 translation)
{
	m4 result = M4Identity();

	result.m03 = translation.x;
	result.m13 = translation.y;
	result.m23 = translation.z;
	result.m33 = 1.f;

	return result;
}

internal m4
M4Scale(v3 scale)
{
	m4 result = M4Identity();

	result.m00 = scale.x;
	result.m11 = scale.y;
	result.m22 = scale.z;
	result.m33 = 1.f;

	return result;
}

internal m4
M4RotateAround(f32 angle, v3 normal)
{
	m4 result = M4Identity();

	v3 up = normal.z > 0.999f ? v3(1.f, 0.f, 0.f) : v3(0.f, 0.f, 1.f);

	v3 F = normal;
	v3 R = V3Normalize(V3Cross(up, F));
	v3 U = V3Normalize(V3Cross(F, R));

	result.m00 = R.x;
	result.m10 = R.y;
	result.m20 = R.z;

	result.m01 = U.x;
	result.m11 = U.y;
	result.m21 = U.z;

	result.m02 = F.x;
	result.m12 = F.y;
	result.m22 = F.z;

	return result;
}

internal m4
M4RotateAxis(f32 angle, v3 axis)
{
	m4 result = M4Identity();

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

internal m4
M4RotateQuat(v4 q)
{
	m4 result = M4Identity();

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
	m4 result = M4Identity();

	result = M4MulM4(M4Translate(V3MulF32(origin, -1.f)), result);
	result = M4MulM4(M4RotateQuat(rotation), result);
	result = M4MulM4(M4Scale(scale), result);
	result = M4MulM4(M4Translate(position), result);

	return result;
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
	v3 s = v3(V3Length(v3(m.m00, m.m10, m.m20)),
			  V3Length(v3(m.m01, m.m11, m.m21)),
			  V3Length(v3(m.m02, m.m12, m.m22)));

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
