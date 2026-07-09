#ifndef GRAPHICS_SAMPLER_H
#define GRAPHICS_SAMPLER_H

typedef struct G_Sampler G_Sampler;
struct G_Sampler
{
	VkSampler vk_handle;
	VkFilter filter;

	VkSamplerAddressMode wrap_x;
	VkSamplerAddressMode wrap_y;
	VkSamplerAddressMode wrap_z;
	
	VkBorderColor border_colour;

	u32 bindless;
};

#endif // GRAPHICS_SAMPLER_H
