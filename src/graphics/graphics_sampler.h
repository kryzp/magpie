#ifndef GRAPHICS_SAMPLER_H
#define GRAPHICS_SAMPLER_H

typedef struct GFX_Sampler GFX_Sampler;
struct GFX_Sampler
{
	VkSampler handle;
	VkFilter filter;

	VkSamplerAddressMode wrap_x;
	VkSamplerAddressMode wrap_y;
	VkSamplerAddressMode wrap_z;
	
	VkBorderColor border_colour;

	GFX_BindlessHandle bindless;
};

#endif // GRAPHICS_SAMPLER_H
