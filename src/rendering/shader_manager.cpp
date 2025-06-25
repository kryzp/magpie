#include "shader_manager.h"

#include "core/app.h"
#include "core/common.h"

using namespace mgp;

void ShaderManager::init(App *app)
{
	m_app = app;

	loadShaders();
}

void ShaderManager::destroy()
{
	for (cauto &[name, stage] : m_shaderStageCache)
		delete stage;

	for (cauto &[name, shader] : m_shaderCache)
		delete shader;
}

ShaderStage *ShaderManager::getShaderStage(const std::string &name)
{
	if (m_shaderStageCache.contains(name))
		return m_shaderStageCache.at(name);

	return nullptr;
}

Shader *ShaderManager::getShader(const std::string &name)
{
	if (m_shaderCache.contains(name))
		return m_shaderCache.at(name);

	return nullptr;
}

ShaderStage *ShaderManager::loadShaderStage(const std::string &name, const std::string &path, VkShaderStageFlagBits stageType)
{
	if (m_shaderStageCache.contains(name))
		return m_shaderStageCache.at(name);

	ShaderStage *stage = m_app->getGraphics()->createShaderStage(stageType, path);
	m_shaderStageCache.insert({ name, stage });

	return stage;
}

void ShaderManager::addShader(const std::string &name, Shader *shader)
{
	m_shaderCache.insert({ name, shader });
}

void ShaderManager::loadShaders()
{
	mgp_LOG("Compiling shaders...");

	// shader stages
	{
		// vertex shaders
		loadShaderStage("primitive_vs",							"primitive_vs",							VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("skybox_vs",							"skybox",								VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("fullscreen_triangle_vs",				"fullscreen_triangle_vs",				VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("model_vs",								"model_vs",								VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("model_shadow_map_vs",					"model_shadow_map_vs",					VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("deferred_lighting_point_light_vs",		"deferred_lighting_point_light",		VK_SHADER_STAGE_VERTEX_BIT);

		// fragment shaders
		loadShaderStage("equirectangular_to_cubemap_fs",		"equirectangular_to_cubemap_fs",		VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("irradiance_convolution_fs",			"irradiance_convolution_fs",			VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("prefilter_convolution_fs",				"prefilter_convolution_fs",				VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("brdf_integrator_fs",					"brdf_integrator_fs",					VK_SHADER_STAGE_FRAGMENT_BIT);
//		loadShaderStage("texturedPBR_fs",						"texturedPBR_fs",						VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("texturedPBR_gbuffer_fs",				"texturedPBR_gbuffer_fs",				VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("deferred_lighting_ambient_fs",			"deferred_lighting_ambient_fs",			VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("deferred_lighting_point_light_fs",		"deferred_lighting_point_light",		VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("skybox_fs",							"skybox",								VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("texture_uv_fs",						"texture_uv_fs",						VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("shadow_map_fs",						"shadow_map_fs",						VK_SHADER_STAGE_FRAGMENT_BIT);

		// compute shaders
		loadShaderStage("hdr_tonemapping_cs", "hdr_tonemapping_cs",									VK_SHADER_STAGE_COMPUTE_BIT);
	}

	// effects
	{
		/*
		// PBR
		addShader("texturedPBR", m_app->getGraphics()->createShader(
			sizeof(int)*16,
			{ m_app->getBindlessResources()->getLayout() },
			{
				getShaderStage("model_vs"),
				getShaderStage("texturedPBR_fs")
			}
		));
		*/

		// DEFERRED GEOMETRY PASS
		addShader("texturedPBR_gbuffer", m_app->getGraphics()->createShader(
			sizeof(int)*16,
			{ m_app->getBindlessResources()->getLayout() },
			{
				getShaderStage("model_vs"),
				getShaderStage("texturedPBR_gbuffer_fs")
			}
		));

		// DEFERRED LIGHTING AMBIENT
		addShader("deferred_lighting_ambient", m_app->getGraphics()->createShader(
			sizeof(int)*12 + sizeof(float)*4,
			{ m_app->getBindlessResources()->getLayout() },
			{
				getShaderStage("fullscreen_triangle_vs"),
				getShaderStage("deferred_lighting_ambient_fs")
			}
		));

		// DEFERRED LIGHTING POINT LIGHT
		addShader("deferred_lighting_point_light", m_app->getGraphics()->createShader(
			2*sizeof(VkDeviceAddress) + 16*sizeof(float) + 8*sizeof(uint32_t),
			{ m_app->getBindlessResources()->getLayout() },
			{
				getShaderStage("deferred_lighting_point_light_vs"),
				getShaderStage("deferred_lighting_point_light_fs")
			}
		));

		// SHADOW MAPPING
		{
			DescriptorLayout *layout = m_app->getDescriptorLayouts().fetchLayout(VK_SHADER_STAGE_ALL_GRAPHICS, { }, 0);

			addShader("shadow_map", m_app->getGraphics()->createShader(
				sizeof(float)*16,
				{ layout },
				{
					getShaderStage("model_shadow_map_vs"),
					getShaderStage("shadow_map_fs")
				}
			));
		}

		// SKYBOX
		{
			DescriptorLayout *layout = m_app->getDescriptorLayouts().fetchLayout(VK_SHADER_STAGE_ALL_GRAPHICS, { DescriptorLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) }, 0);
			
			addShader("skybox", m_app->getGraphics()->createShader(
				sizeof(float)*16 * 2,
				{ layout },
				{
					getShaderStage("skybox_vs"),
					getShaderStage("skybox_fs")
				}
			));
		}

		// EQUIRECTANGULAR TO CUBEMAP
		{
			DescriptorLayout *layout = m_app->getDescriptorLayouts().fetchLayout(VK_SHADER_STAGE_ALL_GRAPHICS, { DescriptorLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) }, 0);
			
			addShader("equirectangular_to_cubemap", m_app->getGraphics()->createShader(
				sizeof(float)*16 * 2,
				{ layout },
				{
					getShaderStage("primitive_vs"),
					getShaderStage("equirectangular_to_cubemap_fs")
				}
			));
		}

		// IRRADIANCE CONVOLUTION
		{
			DescriptorLayout *layout = m_app->getDescriptorLayouts().fetchLayout(VK_SHADER_STAGE_ALL_GRAPHICS, { DescriptorLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) }, 0);
			
			addShader("irradiance_convolution", m_app->getGraphics()->createShader(
				sizeof(float)*16 * 2,
				{ layout },
				{
					getShaderStage("primitive_vs"),
					getShaderStage("irradiance_convolution_fs")
				}
			));
		}

		// PREFILTER CONVOLUTION
		{
			DescriptorLayout *layout = m_app->getDescriptorLayouts().fetchLayout(
				VK_SHADER_STAGE_ALL_GRAPHICS,
				{
					DescriptorLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC),
					DescriptorLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				},
				0
			);

			addShader("prefilter_convolution", m_app->getGraphics()->createShader(
				sizeof(float)*16 * 2,
				{ layout },
				{
					getShaderStage("primitive_vs"),
					getShaderStage("prefilter_convolution_fs")
				}
			));
		}

		// BRDF LUT GENERATION
		{
			DescriptorLayout *layout = m_app->getDescriptorLayouts().fetchLayout(VK_SHADER_STAGE_ALL_GRAPHICS, { }, 0);
			
			addShader("brdf_lut", m_app->getGraphics()->createShader(
				0,
				{ layout },
				{
					getShaderStage("fullscreen_triangle_vs"),
					getShaderStage("brdf_integrator_fs")
				}
			));
		}

		// HDR TONEMAPPING
		{
			DescriptorLayout *layout = m_app->getDescriptorLayouts().fetchLayout(VK_SHADER_STAGE_COMPUTE_BIT, { DescriptorLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ) }, 0);
			
			addShader("hdr_tonemapping", m_app->getGraphics()->createShader(
				sizeof(uint32_t)*2 + sizeof(float),
				{ layout },
				{ getShaderStage("hdr_tonemapping_cs") }
			));
		}

		// TEXTURE UV
		{
			DescriptorLayout *layout = m_app->getDescriptorLayouts().fetchLayout(VK_SHADER_STAGE_ALL_GRAPHICS, { DescriptorLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ) }, 0);

			addShader("texture_uv", m_app->getGraphics()->createShader(
				0,
				{ layout },
				{
					getShaderStage("fullscreen_triangle_vs"),
					getShaderStage("texture_uv_fs")
				}
			));
		}
	}
}
