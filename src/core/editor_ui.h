#pragma once

namespace mgp
{
	class Platform;
	class VulkanCore;
	class CommandBuffer;
	class RenderInfo;
}

namespace mgp::editor_ui
{
	void init(Platform *sdl, VulkanCore *vulkan);
	void update();
	void render(CommandBuffer &cmd, const RenderInfo &info);
}
