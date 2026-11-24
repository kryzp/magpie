#ifndef CORE_VEC3_H
#define CORE_VEC3_H

struct Vec2;

struct Vec3 {
	float x;
	float y;
	float z;

	Vec3();
	Vec3(float x, float y, float z);
	Vec3(const Vec2 &xy, float z);
	Vec3(float s);

	static Vec3 SphericalToCartesian(float radius, float azimuth, float elevation);
	static Vec3 RandomDirection();

	[[nodiscard]] Vec2 XY() const;

	[[nodiscard]] float Dot(const Vec3 &other) const;
	[[nodiscard]] Vec3 Cross(const Vec3 &other) const;

	[[nodiscard]] float LengthSqr() const;
	[[nodiscard]] float Length() const;

	[[nodiscard]] float Distance(const Vec3 &other) const;
	[[nodiscard]] float DistanceSqr(const Vec3 &other) const;

	[[nodiscard]] Vec3 Normalized() const;

	[[nodiscard]] float MaxComponent() const;
	[[nodiscard]] float MinComponent() const;

	[[nodiscard]] Vec3 MinOf(const Vec3 &other) const;
	[[nodiscard]] Vec3 MaxOf(const Vec3 &other) const;

	[[nodiscard]] Vec3 Lerp(const Vec3 &to, float amount) const;
	[[nodiscard]] Vec3 Approach(const Vec3 &to, float amount) const;

	[[nodiscard]] Vec3 Reflect(const Vec3 &normal) const;
	[[nodiscard]] Vec3 Refract(const Vec3 &normal, double eta21) const;

	[[nodiscard]] Vec3 Limit(float min_length, float max_length) const;

	[[nodiscard]] bool IsZero() const;
	[[nodiscard]] bool IsZero(float margin) const;
	[[nodiscard]] Vec3 Limited(float max_length) const;

	Vec3 operator+(const Vec3 &other) const
	{
		return Vec3(x + other.x, y + other.y, z + other.z);
	}

	Vec3 operator-(const Vec3 &other) const
	{
		return Vec3(x - other.x, y - other.y, z - other.z);
	}

	Vec3 operator*(const Vec3 &other) const
	{
		return Vec3(x * other.x, y * other.y, z * other.z);
	}

	Vec3 operator/(const Vec3 &other) const
	{
		return Vec3(x / other.x, y / other.y, z / other.z);
	}

	Vec3 operator*(float f) const
	{
		return Vec3(x * f, y * f, z * f);
	}

	Vec3 operator/(float f) const
	{
		return Vec3(x / f, y / f, z / f);
	}

	Vec3 operator-() const
	{
		return Vec3(-x, -y, -z);
	}

	Vec3 &operator+=(const Vec3 &other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	Vec3 &operator-=(const Vec3 &other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	Vec3 &operator*=(float f)
	{
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	Vec3 &operator/=(float f)
	{
		x /= f;
		y /= f;
		z /= f;
		return *this;
	}

	bool operator==(const Vec3 &other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}

	bool operator!=(const Vec3 &other) const
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

inline Vec3 operator*(float f, const Vec3 &v)
{
	return v * f;
}

#endif // CORE_VEC3_H
