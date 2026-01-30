#pragma once

#include <volk/volk.h>

#include "bindless.h"

namespace gfx
{
	class Sampler {
		friend class Device;

	public:
		static Sampler *linear;

		Sampler()
			: handle()
			, filter()
			, wrap_x()
			, wrap_y()
			, wrap_z()
			, border_colour()
			, bindless_handle()
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

		BindlessHandle get_bindless_handle() const
		{
			return bindless_handle;
		}

	private:
		VkSampler handle;
		VkFilter filter;

		VkSamplerAddressMode wrap_x;
		VkSamplerAddressMode wrap_y;
		VkSamplerAddressMode wrap_z;

		VkBorderColor border_colour;

		BindlessHandle bindless_handle;
	};
}
