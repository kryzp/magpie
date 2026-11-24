#ifndef CORE_QUAT_H
#define CORE_QUAT_H

struct Vec3;
struct Vec4;

// euler assumes Vec3(pitch, yaw, roll)
struct Quat {
	float x;
	float y;
	float z;
	float w;

	Quat();
	Quat(float x, float y, float z, float w);

	static Quat Identity()
	{
		return Quat();
	}

	static Quat FromAxis(float angle, const Vec3 &axis);
	static Quat FromEuler(const Vec3 &euler);
	static Quat LookAt(const Vec3 &eye, const Vec3 &centre);

	[[nodiscard]] Vec4 AsVec4() const;

	[[nodiscard]] float Dot(const Quat &other) const;
	[[nodiscard]] float LengthSqr() const;
	[[nodiscard]] float Length() const;

	[[nodiscard]] Vec3 ToEuler() const;
	[[nodiscard]] Quat Inverse() const;

	[[nodiscard]] Quat Slerp(const Quat &to, float t) const;

	bool operator==(const Quat &other) const
	{
		return x == other.x && y == other.y && z == other.z && w == other.w;
	}

	bool operator!=(const Quat &other) const
	{
		return !(*this == other);
	}
};

#endif // CORE_QUAT_H
