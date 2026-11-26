#include "core/maths.h"
#include "core/hash.h"

#include <random>

static std::mt19937_64 random_engine(0x6d61677069650001ull);

float maths::NormalizeAngle(float radians)
{
	radians = std::fmod(radians, PI2f);

	if (radians < 0.f)
		radians += PI2f;

	return radians;
}

float maths::LerpAngle(float from_radians, float to_radians, float t)
{
	float delta = std::fmod(to_radians - from_radians + PI2f + PIf, PI2f) - PIf;
	return std::fmod(from_radians + delta * t + PI2f, PI2f);
}

int maths::FloorDiv(int a, int b)
{
	int q = a / b;

	if ((a % b != 0) && ((a < 0) != (b < 0)))
		q--;

	return q;
}

int maths::FloorMod(int a, int b)
{
	int m = a % b;

	if (m < 0) {
		m += b;
	}

	return m;
}

float maths::Quantize(float value, float resolution)
{
	return static_cast<float>(std::lround(value * resolution)) / resolution;
}

float maths::WrapPi(float radians)
{
	return radians - PI2f * static_cast<float>(std::lround(radians / PI2f));
}

float maths::AngleDistance(float from_radians, float to_radians)
{
	float delta = to_radians - from_radians;

	return std::fabs(std::atan2(std::sin(delta), std::cos(delta)));
}

float maths::SmoothStep(float t)
{
	return t * t * (3.f - 2.f * t);
}

float maths::HashSigned(int a, int b)
{
	u32 h = hash::Ints(a, b);
	return static_cast<float>(h & 0xFFFFu) / 32767.5f - 1.f;
}

float maths::ValueNoise(float x, int seed)
{
	int i = Floor(x);
	float u = SmoothStep(x - static_cast<float>(i));

	float a = HashSigned(i, seed);
	float b = HashSigned(i + 1, seed);

	return a + (b - a) * u;
}

float maths::Random01()
{
	std::uniform_real_distribution<float> dist(0.f, 1.f);
	return dist(random_engine);
}

float maths::Random(float lo, float hi)
{
	return lo + Random01() * (hi - lo);
}

int maths::RandomInt(int lo, int hi)
{
	std::uniform_int_distribution<int> dist(lo, hi);
	return dist(random_engine);
}

bool maths::RandomBool()
{
	return Random01() < 0.5f;
}

float maths::RandomSign()
{
	return RandomBool() ? 1.f : -1.f;
}

void maths::SeedRandom(u64 seed)
{
	random_engine.seed(seed);
}
