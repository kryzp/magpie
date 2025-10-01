#ifndef GFX_SAMPLER_H
#define GFX_SAMPLER_H

#include <volk/volk.h>

#include "bindless.h"

struct gfx_sampler {
	VkSampler handle;
	struct gfx_bindless_sampler bindless;
	VkFilter filter;
	VkSamplerAddressMode wrap_x;
	VkSamplerAddressMode wrap_y;
	VkSamplerAddressMode wrap_z;
	VkBorderColor border_colour;
};

#endif // GFX_SAMPLER_H
