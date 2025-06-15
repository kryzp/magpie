#pragma once

#include <Volk/volk.h>

namespace mgp
{
	class GraphicsCore;
	class PlatformCore;

	class Surface
	{
	public:
		Surface() = default;
		~Surface() = default;

		void create(GraphicsCore *gfx, const PlatformCore *platform);
		void destroy();

		const VkSurfaceKHR &getHandle() const;

	private:
		GraphicsCore *m_gfx;
		VkSurfaceKHR m_surface;
	};
}
