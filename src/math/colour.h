#pragma once

#include <glm/vec4.hpp>

#include "core/common.h"

namespace mgp
{
	struct Colour
	{
		union
		{
			struct
			{
				uint8_t a;
				uint8_t r;
				uint8_t g;
				uint8_t b;
			};

			uint8_t data[4];
		};

		Colour();
		Colour(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
		Colour(uint32_t packed);

		static const Colour &empty();
		static const Colour &black();
		static const Colour &white();
		static const Colour &red();
		static const Colour &green();
		static const Colour &blue();
		static const Colour &yellow();
		static const Colour &magenta();
		static const Colour &cyan();

		static Colour fromHSV(float hue, float sat, float val, uint8_t alpha = 255);

		static Colour lerp(const Colour &from, const Colour &to, float amount);

		uint32_t getPacked() const;

		Colour getPremultiplied() const;

		glm::vec4 getDisplayColour() const;

		void exportToUint8(uint8_t *colours) const;
		void exportToFloat(float *colours) const;

		bool operator == (const Colour &other) const;
		bool operator != (const Colour &other) const;

		Colour operator - () const;
		Colour operator * (float factor) const;
		Colour operator / (float factor) const;

		Colour &operator *= (float factor);
		Colour &operator /= (float factor);
	};
}
