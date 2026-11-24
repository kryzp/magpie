#ifndef CORE_VEC4_H
#define CORE_VEC4_H

struct Vec3;

struct Vec4 {
	float x;
	float y;
	float z;
	float w;

	Vec4();
	Vec4(float x, float y, float z, float w);
	Vec4(const Vec3 &xyz, float w);
	Vec4(float s);

	[[nodiscard]] Vec3 XYZ() const;

	[[nodiscard]] float Dot(const Vec4 &other) const;

	[[nodiscard]] float LengthSqr() const;
	[[nodiscard]] float Length() const;

	[[nodiscard]] Vec4 Lerp(const Vec4 &to, float t) const;

	[[nodiscard]] Vec4 FrustumNormalizePlane() const;

	Vec4 operator+(const Vec4 &other) const
	{
		return Vec4(x + other.x, y + other.y, z + other.z, w + other.w);
	}

	Vec4 operator-(const Vec4 &other) const
	{
		return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
	}

	Vec4 operator*(const Vec4 &other) const
	{
		return Vec4(x * other.x, y * other.y, z * other.z, w * other.w);
	}

	Vec4 operator*(float f) const
	{
		return Vec4(x * f, y * f, z * f, w * f);
	}

	Vec4 &operator+=(const Vec4 &other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;
		return *this;
	}

	Vec4 &operator*=(float f)
	{
		x *= f;
		y *= f;
		z *= f;
		w *= f;
		return *this;
	}

	bool operator==(const Vec4 &other) const
	{
		return x == other.x && y == other.y && z == other.z && w == other.w;
	}

	bool operator!=(const Vec4 &other) const
	{
		return !(*this == other);
	}

	float &operator[](unsigned i)
	{
		return (&x)[i];
	}

	float operator[](unsigned i) const
	{
		return (&x)[i];
	}
};

#endif // CORE_VEC4_H
