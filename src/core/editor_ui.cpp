#include "editor_ui.h"

#include "platform.h"

#include "vulkan/core.h"
#include "vulkan/render_info.h"
#include "vulkan/command_buffer.h"
#include "vulkan/swapchain.h"

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_sdl3.h"
#include "third_party/imgui/imgui_impl_vulkan.h"

using namespace mgp;

void editor_ui::init(Platform *sdl, VulkanCore *vulkan)
{
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	//ImGuiIO &io = ImGui::GetIO();

	ImGui::StyleColorsClassic();

	sdl->initImGui();
	vulkan->initImGui();
}

void editor_ui::update()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();

	ImGui::NewFrame();
}

void editor_ui::render(CommandBuffer &cmd, const RenderInfo &info)
{
	ImGui::Render();

	ImGui_ImplVulkan_RenderDrawData(
		ImGui::GetDrawData(),
		cmd.getHandle()
	);
}
