#ifndef CORE_MATHS_H
#define CORE_MATHS_H

#include "core/types.h"

#include <cmath>
#include <cfloat>

namespace maths {

constexpr double PI = 3.1415926535897;
constexpr float PIf = 3.1415926535897f;
constexpr float PI2f = PIf * 2.f;
constexpr float HALF_PIf = PIf * 0.5f;

constexpr double ONE_OVER_SQRT_TWO_PI = 0.3989422804;
constexpr float ONE_OVER_SQRT_TWO_PIf = 0.3989422804f;

constexpr double EULER = 2.718281828459045;
constexpr float EULERf = 2.718281828459045f;

constexpr double DEG_TO_RAD = 0.0174532925199;
constexpr float DEG_TO_RADf = 0.0174532925199f;

constexpr double RAD_TO_DEG = 57.2957795131;
constexpr float RAD_TO_DEGf = 57.2957795131f;

constexpr double EPSILON = DBL_EPSILON;
constexpr float EPSILONf = FLT_EPSILON;

constexpr float MAX_FLOAT = FLT_MAX;
constexpr double MAX_DOUBLE = DBL_MAX;

template <typename T>
inline T Abs(T a)
{
	return (a < T(0)) ? -a : a;
}

template <typename T>
inline T Min(T a, T b)
{
	return (a < b) ? a : b;
}

template <typename T>
inline T Max(T a, T b)
{
	return (a > b) ? a : b;
}

template <typename T>
inline T Clamp(T v, T lo, T hi)
{
	return Max(lo, Min(hi, v));
}

template <typename T>
inline T Saturate(T v)
{
	return Clamp(v, T(0), T(1));
}

inline bool WithinEpsilon(double v)
{
	return Abs(v) < EPSILON;
}

inline float Lerp(float from, float to, float t)
{
	return from + (to - from) * t;
}

inline float Approach(float from, float to, float amount)
{
	return (to > from) ? Min(from + amount, to) : Max(from - amount, to);
}

float LerpAngle(float from_radians, float to_radians, float t);
float NormalizeAngle(float radians);

inline int Floor(float v)
{
	return static_cast<int>(std::floor(v));
}

inline int Ceil(float v)
{
	return static_cast<int>(std::ceil(v));
}

template <typename T>
inline T Sign(T v)
{
	if (v > T(0)) {
		return T(1);
	}

	if (v < T(0)) {
		return T(-1);
	}

	return T(0);
}

int FloorDiv(int a, int b);
int FloorMod(int a, int b);

float Quantize(float value, float resolution);

float WrapPi(float radians);

float AngleDistance(float from_radians, float to_radians);

float SmoothStep(float t);

float HashSigned(int a, int b);

float ValueNoise(float x, int seed);

float Random01();
float Random(float lo, float hi);
int RandomInt(int lo, int hi); // inclusive both ends
bool RandomBool();
float RandomSign();
void SeedRandom(u64 seed);

}

#endif // CORE_MATHS_H
