#include "debug_ui.h"

#include "platform.h"

#include "vulkan/core.h"
#include "vulkan/render_info.h"
#include "vulkan/command_buffer.h"
#include "vulkan/swapchain.h"

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_sdl3.h"
#include "third_party/imgui/imgui_impl_vulkan.h"

using namespace mgp;

static Platform *g_sdl;
static VulkanCore *g_vulkan;

void dbgui::init(Platform *sdl, VulkanCore *vulkan)
{
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	//ImGuiIO &io = ImGui::GetIO();

	ImGui::StyleColorsClassic();

	g_sdl = sdl;
	g_vulkan = vulkan;

	g_sdl->initImGui();
	g_vulkan->initImGui();
}

void dbgui::update()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	/*
	ImGui::Begin("Material System");
	{
		static float intensity = 0.0f;

		if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 100.0f))
		{
			Light light = {};
			light.position = { 0.0f, 5.0f, 0.0f };
			light.radius = 0.0f;
			light.attenuation = 0.0f;
			light.colour = { intensity, intensity, intensity };
			light.direction = { 1.0f, -1.0f, -1.0f };
			light.type = LIGHT_TYPE_SUN;
			light.direction /= glm::length(light.direction);

			g_materialSystem->m_globalData.lights[0] = light;
			g_materialSystem->pushGlobalData();
		}
	}
	ImGui::End();
	*/

	/*
	ImGui::Begin("Post Processing");
	{
		if (ImGui::SliderFloat("HDR Exposure", &g_exposure, 0.0f, 5.0f))
		{
			g_postProcessPass.setExposure(g_exposure);
		}

		if (ImGui::SliderFloat("Bloom Radius", &g_bloomRadius, 0.0f, 0.02f))
		{
			g_postProcessPass.setBloomRadius(g_bloomRadius);
		}

		if (ImGui::SliderFloat("Bloom Intensity", &g_bloomIntensity, 0.0f, 0.1f))
		{
			g_postProcessPass.setBloomIntensity(g_bloomIntensity);
		}
	}
	ImGui::End();
	*/

	ImGui::ShowDemoWindow();
}

void dbgui::render(CommandBuffer &cmd, const Swapchain *swapchain)
{
	ImGui::Render();

	RenderInfo renderInfo(g_vulkan);
	renderInfo.setSize(swapchain->getWidth(), swapchain->getHeight());

	renderInfo.addColourAttachment(
		VK_ATTACHMENT_LOAD_OP_LOAD,
		*swapchain->getCurrentSwapchainImageView()
	);

	cmd.beginRendering(renderInfo);
	{
		ImDrawData *drawData = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(drawData, cmd.getHandle());
	}
	cmd.endRendering();
}
