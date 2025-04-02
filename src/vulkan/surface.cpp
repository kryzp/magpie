#include "surface.h"

#include "core/platform.h"

#include "core.h"

using namespace mgp;

void Surface::create(const VulkanCore *core, const Platform *platform)
{
	m_core = core;

	if (bool result = platform->vkCreateSurface(m_core->getInstance(), &m_surface); !result) {
		MGP_ERROR("Failed to create surface: %d", result);
	}
}

void Surface::destroy()
{
	vkDestroySurfaceKHR(m_core->getInstance(), m_surface, nullptr);
	m_surface = VK_NULL_HANDLE;
}

const VkSurfaceKHR &Surface::getHandle() const
{
	return m_surface;
}
