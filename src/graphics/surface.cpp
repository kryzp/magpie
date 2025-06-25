#include "surface.h"

#include "platform/platform_core.h"

#include "core/common.h"

#include "graphics_core.h"

using namespace mgp;

void Surface::create(GraphicsCore *gfx, const PlatformCore *platform)
{
	m_gfx = gfx;

	if (!platform->vkCreateSurface((void *)gfx->getInstance(), (void **)(&m_surface)))
		mgp_ERROR("Failed to create surface");

	mgp_LOG("Created surface!");
}

void Surface::destroy()
{
	vkDestroySurfaceKHR(m_gfx->getInstance(), m_surface, nullptr);
	m_surface = VK_NULL_HANDLE;
}

const VkSurfaceKHR &Surface::getHandle() const
{
	return m_surface;
}
