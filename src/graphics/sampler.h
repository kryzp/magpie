#ifndef GRAPHICS_SAMPLER_H
#define GRAPHICS_SAMPLER_H

#include "core/types.h"

#include <volk/volk.h>

namespace gfx {
struct SamplerCreateInfo {
	VkFilter filter = VK_FILTER_LINEAR;
	VkSamplerAddressMode wrap_x = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VkSamplerAddressMode wrap_y = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VkSamplerAddressMode wrap_z = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VkBorderColor border_colour = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	float max_lod = VK_LOD_CLAMP_NONE; // 0 samples the top mip only
	bool anisotropy = true;
	bool compare = false;
	VkCompareOp compare_op = VK_COMPARE_OP_ALWAYS;
};

struct Sampler {
	VkSampler vk_handle = VK_NULL_HANDLE;
	VkFilter filter = VK_FILTER_LINEAR;

	VkSamplerAddressMode wrap_x = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VkSamplerAddressMode wrap_y = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VkSamplerAddressMode wrap_z = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	VkBorderColor border_colour = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

	unsigned bindless = 0;
};
}

#endif // GRAPHICS_SAMPLER_H
