#include "colour.h"
#include "calc.h"

Colour::Colour()
	: r(0)
	, g(0)
	, b(0)
	, a(0)
{
}

Colour::Colour(u8 r, u8 g, u8 b, u8 a)
	: r(r)
	, g(g)
	, b(b)
	, a(a)
{
}

Colour::Colour(u32 packed)
	: a(static_cast<u8>(packed >> 0 ))
	, r(static_cast<u8>(packed >> 8 ))
	, g(static_cast<u8>(packed >> 16))
	, b(static_cast<u8>(packed >> 24))
{
}

u32 Colour::packed() const
{
	return (
		a << 0  |
		r << 8  |
		g << 16 |
		b << 24
	);
}

Colour Colour::from_hsv(float hue, float sat, float val, u8 alpha)
{
	float C = sat * val;
	float X = C * (1.0f - CalcF::abs(CalcF::mod(hue / 60.0f, 2.0f) - 1.0f));
	float m = val - C;

	float r, g, b;
	r = g = b = 0.0f;

	if (0 <= hue && hue < 60)
	{
		r = C;
		g = X;
		b = 0.0f;
	}
	else if (60 <= hue && hue < 120)
	{
		r = X;
		g = C;
		b = 0.0f;
	}
	else if (120 <= hue && hue < 180)
	{
		r = 0.0f;
		g = C;
		b = X;
	}
	else if (180 <= hue && hue < 240)
	{
		r = 0.0f;
		g = X;
		b = C;
	}
	else if (240 <= hue && hue < 300)
	{
		r = X;
		g = 0.0f;
		b = C;
	}
	else if (300 <= hue && hue < 360)
	{
		r = C;
		g = 0.0f;
		b = X;
	}

	return Colour(
		static_cast<u8>((r + m) * 255.0f),
		static_cast<u8>((g + m) * 255.0f),
		static_cast<u8>((b + m) * 255.0f),
		alpha
	);
}

Colour Colour::lerp(const Colour &from, const Colour &to, float amount)
{
	return Colour(
		CalcF::lerp(from.r, to.r, amount),
		CalcF::lerp(from.g, to.g, amount),
		CalcF::lerp(from.b, to.b, amount),
		CalcF::lerp(from.a, to.a, amount)
	);
}

Colour Colour::premultiplied() const
{
	return Colour(
		r * a / 255,
		g * a / 255,
		b * a / 255
	);
}

void Colour::export_to_u8(u8 *colours) const
{
	colours[0] = r;
	colours[1] = g;
	colours[2] = b;
	colours[3] = a;
}

void Colour::export_to_float(float *colours) const
{
	colours[0] = static_cast<float>(r) / 255.0f;
	colours[1] = static_cast<float>(g) / 255.0f;
	colours[2] = static_cast<float>(b) / 255.0f;
	colours[3] = static_cast<float>(a) / 255.0f;
}

bool Colour::operator == (const Colour& other) const
{
	return (
		r == other.r &&
		g == other.g &&
		b == other.b &&
		a == other.a
	);
}

bool Colour::operator != (const Colour& other) const
{
	return !(*this == other);
}

Colour Colour::operator - () const
{
	return Colour(
		255 - r,
		255 - g,
		255 - b,
		a
	);
}

Colour Colour::operator * (float factor) const
{
	return Colour(
		static_cast<u8>(static_cast<float>(r) * factor),
		static_cast<u8>(static_cast<float>(g) * factor),
		static_cast<u8>(static_cast<float>(b) * factor),
		static_cast<u8>(static_cast<float>(a) * factor)
	);
}

Colour Colour::operator / (float factor) const
{
	return Colour(
		static_cast<u8>(static_cast<float>(r) / factor),
		static_cast<u8>(static_cast<float>(g) / factor),
		static_cast<u8>(static_cast<float>(b) / factor),
		static_cast<u8>(static_cast<float>(a) / factor)
	);
}

Colour &Colour::operator *= (float factor)
{
	(*this) = (*this) * factor;
	return *this;
}

Colour &Colour::operator /= (float factor)
{
	(*this) = (*this) / factor;
	return *this;
}

const Colour &Colour::empty()   { static const Colour EMPTY   = Colour(000, 000, 000, 000); return EMPTY;   }
const Colour &Colour::black()   { static const Colour BLACK   = Colour(000, 000, 000, 255); return BLACK;   }
const Colour &Colour::white()   { static const Colour WHITE   = Colour(255, 255, 255, 255); return WHITE;   }
const Colour &Colour::red()     { static const Colour RED     = Colour(255, 000, 000, 255); return RED;     }
const Colour &Colour::green()   { static const Colour GREEN   = Colour(000, 255, 000, 255); return GREEN;   }
const Colour &Colour::blue()    { static const Colour BLUE    = Colour(000, 000, 255, 255); return BLUE;    }
const Colour &Colour::yellow()  { static const Colour YELLOW  = Colour(255, 255, 000, 255); return YELLOW;  }
const Colour &Colour::magenta() { static const Colour MAGENTA = Colour(255, 000, 255, 255); return MAGENTA; }
const Colour &Colour::cyan()    { static const Colour CYAN    = Colour(000, 255, 255, 255); return CYAN;    }
