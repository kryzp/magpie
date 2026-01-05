#pragma once

#include <volk/volk.h>

#include "bindless.h"

namespace gfx
{

class Sampler {
	friend class Device;

public:
	Sampler()
		: handle()
		, filter()
		, wrap_x()
		, wrap_y()
		, wrap_z()
		, border_colour()
		, bindless()
	{
	}

	~Sampler() = default;

	const VkSampler &get_handle() const
	{
		return handle;
	}

	VkFilter get_filter() const
	{
		return filter;
	}

	VkSamplerAddressMode get_wrap_x() const
	{
		return wrap_x;
	}

	VkSamplerAddressMode get_wrap_y() const
	{
		return wrap_y;
	}

	VkSamplerAddressMode get_wrap_z() const
	{
		return wrap_z;
	}

	VkBorderColor get_border_colour() const
	{
		return border_colour;
	}

	const BindlessSampler &get_bindless() const
	{
		return bindless;
	}

private:
	VkSampler handle;
	VkFilter filter;

	VkSamplerAddressMode wrap_x;
	VkSamplerAddressMode wrap_y;
	VkSamplerAddressMode wrap_z;

	VkBorderColor border_colour;

	BindlessSampler bindless;
};

}
