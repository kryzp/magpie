
internal Sampler SamplerInit(VkFilter filter, VkSamplerAddressMode wrap_x,
			     VkSamplerAddressMode wrap_y,
			     VkSamplerAddressMode wrap_z,
			     VkBorderColor border_colour)
{
	VkPhysicalDeviceProperties properties =	graphics_device->physical_device_properties.properties;

	VkSamplerCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	create_info.minFilter = filter;
	create_info.magFilter = filter;
	create_info.addressModeU = wrap_x;
	create_info.addressModeV = wrap_z;
	create_info.addressModeW = wrap_z;
	create_info.anisotropyEnable = VK_TRUE;
	create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	create_info.borderColor = border_colour;
	create_info.unnormalizedCoordinates = VK_FALSE;
	create_info.compareEnable = VK_FALSE;
	create_info.compareOp = VK_COMPARE_OP_ALWAYS;
	create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	create_info.mipLodBias = 0.f;
	create_info.minLod = 0.f;
	create_info.maxLod = VK_LOD_CLAMP_NONE;

	Sampler sampler = {0};
	sampler.filter = filter;
	sampler.wrap_x = wrap_x;
	sampler.wrap_y = wrap_y;
	sampler.wrap_z = wrap_z;
	sampler.border_colour = border_colour;

	VK_CHECK(vkCreateSampler(graphics_device->device, &create_info, 0,
				 &sampler.handle),
		 "Failed to create texture sampler.");

	sampler.resource_id = BindlessRegisterSampler(&graphics_device->bindless, sampler.handle);

	return sampler;
}

internal Sampler SamplerInitFilter(VkFilter filter)
{
	return SamplerInit(filter,
			   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			   VK_BORDER_COLOR_INT_OPAQUE_BLACK);
}

internal void SamplerDestroy(Sampler *sampler)
{
	vkDestroySampler(graphics_device->device, sampler->handle, NULL);
	sampler->handle = VK_NULL_HANDLE;
}
