#pragma once

#include "third_party/volk.h"

namespace mgp
{
	class Platform;
	class VulkanCore;

	class Surface
	{
	public:
		Surface() = default;
		~Surface() = default;

		void create(const VulkanCore *core, const Platform *platform);
		void destroy();

		const VkSurfaceKHR &getHandle() const;

	private:
		VkSurfaceKHR m_surface;

		const VulkanCore *m_core;
	};
}
