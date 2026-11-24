#ifndef CORE_VEC2_H
#define CORE_VEC2_H

struct Vec2 {
	float x;
	float y;

	Vec2();
	Vec2(float x, float y);
	Vec2(float s);

	static Vec2 FromAngleRad(float radians, float length);

	static const Vec2 &zero()
	{
		static const Vec2 ZERO(0.f, 0.f);
		return ZERO;
	}

	[[nodiscard]] float Dot(const Vec2 &other) const;
	[[nodiscard]] float Cross(const Vec2 &other) const;

	[[nodiscard]] float LengthSqr() const;
	[[nodiscard]] float Length() const;

	[[nodiscard]] float Distance(const Vec2 &other) const;
	[[nodiscard]] float DistanceSqr(const Vec2 &other) const;

	[[nodiscard]] Vec2 Normalized() const;
	[[nodiscard]] Vec2 WithLength(float length) const;
	[[nodiscard]] Vec2 Limited(float hi) const;
	[[nodiscard]] Vec2 Limited2(float max_length_sqr) const;
	[[nodiscard]] Vec2 Clamped(float lo, float hi) const;

	[[nodiscard]] Vec2 Lerp(const Vec2 &to, float t) const;

	[[nodiscard]] Vec2 RotatedRad(float radians) const;
	[[nodiscard]] float AngleRad() const;
	[[nodiscard]] bool IsZero() const;
	[[nodiscard]] bool IsZero(float margin) const;

	Vec2 operator+(const Vec2 &other) const
	{
		return Vec2(x + other.x, y + other.y);
	}

	Vec2 operator-(const Vec2 &other) const
	{
		return Vec2(x - other.x, y - other.y);
	}

	Vec2 operator*(const Vec2 &other) const
	{
		return Vec2(x * other.x, y * other.y);
	}

	Vec2 operator/(const Vec2 &other) const
	{
		return Vec2(x / other.x, y / other.y);
	}

	Vec2 operator*(float f) const
	{
		return Vec2(x * f, y * f);
	}

	Vec2 operator/(float f) const
	{
		return Vec2(x / f, y / f);
	}

	Vec2 operator-() const
	{
		return Vec2(-x, -y);
	}

	Vec2 &operator+=(const Vec2 &other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	Vec2 &operator-=(const Vec2 &other)
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}

	Vec2 &operator*=(float f)
	{
		x *= f;
		y *= f;
		return *this;
	}

	Vec2 &operator/=(float f)
	{
		x /= f;
		y /= f;
		return *this;
	}

	bool operator==(const Vec2 &other) const
	{
		return x == other.x && y == other.y;
	}

	bool operator!=(const Vec2 &other) const
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

inline Vec2 operator*(float f, const Vec2 &v)
{
	return v * f;
}

#endif // CORE_VEC2_H
