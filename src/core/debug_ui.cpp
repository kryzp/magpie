#include "debug_ui.h"

#include "rendering/material_system.h"
#include "rendering/light.h"

#include "rendering/passes/post_process_pass.h"

#include "third_party/imgui/imgui.h"

using namespace llt;

static float g_exposure;
static float g_bloomRadius;
static float g_bloomIntensity;

void dbgui::init()
{
	g_exposure = g_postProcessPass.getExposure();
	g_bloomRadius = g_postProcessPass.getBloomRadius();
	g_bloomIntensity = g_postProcessPass.getBloomIntensity();
}

void dbgui::update()
{
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

	ImGui::ShowDemoWindow();
}
