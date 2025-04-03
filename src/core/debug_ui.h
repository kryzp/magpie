#pragma once

namespace mgp
{
	class Platform;
	class VulkanCore;
	class CommandBuffer;
	class Swapchain;
}

namespace mgp::dbgui
{
	void init(Platform *sdl, VulkanCore *vulkan);
	void update();
	void render(CommandBuffer &cmd, const Swapchain *swapchain);
}
