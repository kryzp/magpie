#pragma once

#include "calc.h"

template <typename T>
class Ease {
public:
	static T linear(T t);

	static T elastic_in(T t);
	static T elastic_out(T t);
	static T elastic_in_out(T t);

	static T quadratic_in(T t);
	static T quadratic_out(T t);
	static T quadratic_in_out(T t);

	static T sine_in(T t);
	static T sine_out(T t);
	static T sine_in_out(T t);

	static T exp_in(T t);
	static T exp_out(T t);
	static T exp_in_out(T t);

	static T back_in(T t);
	static T back_out(T t);
	static T back_in_out(T t);

	static T circ_in(T t);
	static T circ_out(T t);
	static T circ_in_out(T t);
};

using EaseF   = Ease<float>;
using EaseD   = Ease<double>;
using EaseF32 = Ease<float>;
using EaseF64 = Ease<double>;

template <typename T>
T Ease<T>::linear(T t)
{
	return t;
}

template <typename T>
T Ease<T>::elastic_in(T t)
{
	return Calc<T>::sin(13.0 * Calc<T>::PI * 0.5 * t) * Calc<T>::pow(2, 10.0 * (t - 1.0));
}

template <typename T>
T Ease<T>::elastic_out(T t)
{
	return Calc<T>::sin(-13.0 * Calc<T>::PI * 0.5 * (t + 1.0)) * Calc<T>::pow(2.0, -10.0 * t) + 1.0;
}

template <typename T>
T Ease<T>::elastic_in_out(T t)
{
	if (t < 0.5) {
		return 0.5 * Calc<T>::sin(13.0 * Calc<T>::PI * 0.5 * 2.0 * t) * Calc<T>::pow(2.0, 10.0 * ((2.0 * t) - 1.0));
	}

	return 0.5 * (Calc<T>::sin(-13.0 * Calc<T>::PI * 0.5 * (((2.0 * t) - 1.0) + 1.0)) * Calc<T>::pow(2.0, -10.0 * ((2.0 * t) - 1.0)) + 2.0);
}

template <typename T>
T Ease<T>::quadratic_in(T t)
{
	return t * t;
}

template <typename T>
T Ease<T>::quadratic_out(T t)
{
	return -t * (t - 2.0);
}

template <typename T>
T Ease<T>::quadratic_in_out(T t)
{
	if (t < 0.5) {
		return 2.0 * t * t;
	}

	return 1.0 - (2.0 * t * t);
}

template <typename T>
T Ease<T>::sine_in(T t)
{
	return -Calc<T>::cos(t * Calc<T>::PI * 0.5) + 1.0;
}

template <typename T>
T Ease<T>::sine_out(T t)
{
	return Calc<T>::sin(t * Calc<T>::PI * 0.5);
}

template <typename T>
T Ease<T>::sine_in_out(T t)
{
	return (-Calc<T>::cos(t * Calc<T>::PI) * 0.5) + 0.5;
}

template <typename T>
T Ease<T>::exp_in(T t)
{
	return Calc<T>::pow(2.0, 10.0 * (t - 1.0));
}

template <typename T>
T Ease<T>::exp_out(T t)
{
	return -Calc<T>::pow(2.0, -10.0 * t) + 1.0;
}

template <typename T>
T Ease<T>::exp_in_out(T t)
{
	if (t < 0.5) {
		return Calc<T>::pow(2.0, 10.0 * ((2.0 * t) - 1.0)) * 0.5;
	}

	return -(Calc<T>::pow(2.0, -10.0 * ((2.0 * t) - 1.0)) + 2.0) * 0.5;
}

template <typename T>
T Ease<T>::back_in(T t)
{
	return (2.70158 * t * t * t) - (1.70158 * t * t);
}

template <typename T>
T Ease<T>::back_out(T t)
{
	return 1.0 + (2.70158 * Calc<T>::pow(t - 1.0, 3.0) + 1.70158 * Calc<T>::pow(t - 1.0, 2.0));
}

template <typename T>
T Ease<T>::back_in_out(T t)
{
	if (t < 0.5) {
		return (Calc<T>::pow(2.0 * t, 2.0) * ((3.5949095 * 2.0 * t) - 2.5949095)) * 0.5;
	}

	return (Calc<T>::pow((2.0 * t) - 2.0, 2.0) * (3.5949095 * ((2.0 * t) - 2.0) + 2.5949095) + 2.0) * 0.5;
}

template <typename T>
T Ease<T>::circ_in(T t)
{
	return 1.0 - Calc<T>::sqrt(1.0 - Calc<T>::pow(t, 2.0));
}

template <typename T>
T Ease<T>::circ_out(T t)
{
	return Calc<T>::sqrt(1.0 - Calc<T>::pow(t - 1.0, 2.0));
}

template <typename T>
T Ease<T>::circ_in_out(T t)
{
	if (t < 0.5) {
		return (1.0 - Calc<T>::sqrt(1.0 - Calc<T>::pow(2.0 * t, 2.0))) * 0.5;
	}

	return (Calc<T>::sqrt(1 - Calc<T>::pow((-2.0 * t) + 2.0, 2.0)) + 1) * 0.5;
}
