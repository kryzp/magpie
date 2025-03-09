#include "debug_ui.h"

#include "rendering/material_system.h"
#include "rendering/light.h"

#include "third_party/imgui/imgui.h"

using namespace llt;

void debugui::updateImGui()
{
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

	ImGui::ShowDemoWindow();
}
