#pragma once

#include <random>
#include <ctime>

#include "core/types.h"

template <class Engine = std::mt19937>
class Random {
public:
	Random();
	Random(u64 seed);

	~Random() = default;

	void regen_seed(u64 seed);

	int next_int(int min, int max);
	float next_float(float min = 0.0f, float max = 1.0f);
	double next_double(double min = 0.0, double max = 1.0);

	template <typename Dist, typename T>
	T generic_range(T min, T max);

private:
	Engine rng;
};

template <class Engine>
Random<Engine>::Random()
	: rng(std::time(nullptr))
{
}

template <class Engine>
Random<Engine>::Random(u64 seed)
	: rng(seed)
{
}

template <class Engine>
void Random<Engine>::regen_seed(u64 seed)
{
	rng.seed(seed);
}

template <class Engine>
int Random<Engine>::next_int(int min, int max)
{
	return generic_range<std::uniform_int_distribution<int>>(min, max);
}

template <class Engine>
float Random<Engine>::next_float(float min, float max)
{
	return generic_range<std::uniform_real_distribution<float>>(min, max);
}

template <class Engine>
double Random<Engine>::next_double(double min, double max)
{
	return generic_range<std::uniform_real_distribution<double>>(min, max);
}

template <class Engine>
template <typename Dist, typename T>
T Random<Engine>::generic_range(T min, T max)
{
	return Dist(min, max)(rng);
}
