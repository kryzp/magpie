#ifndef COLOUR_H_
#define COLOUR_H_

#include <glm/vec3.hpp>

#include "core/common.h"

namespace llt
{
	/**
	 * Used for representing a 32-bit (0-255 per colour) colour.
	 */
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

		/*
		 * Basic core colours for convenience.
		 */
		static const Colour &empty();
		static const Colour &black();
		static const Colour &white();
		static const Colour &red();
		static const Colour &green();
		static const Colour &blue();
		static const Colour &yellow();
		static const Colour &magenta();
		static const Colour &cyan();

		/*
		 * Build a colour from the HSV representation.
		 */
		static Colour fromHSV(float hue, float sat, float val, uint8_t alpha = 255);

		/*
		 * Linearly interpolate between two colours
		 */
		static Colour lerp(const Colour &from, const Colour &to, float amount);

		/*
		 * Get the packed colour value as a single 32-bit integer.
		 */
		uint32_t getPacked() const;

		/*
		 * Get this colour with the alpha component applied to all other colours.
		 */
		Colour getPremultiplied() const;

		/*
		 * Get the display colour version.
		 */
		glm::vec3 getDisplayColour() const;

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

#endif // COLOUR_H_
