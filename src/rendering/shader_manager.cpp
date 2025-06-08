#include "shader_manager.h"

#include <fstream>

#include "core/common.h"

#include "vulkan/core.h"
#include "vulkan/descriptor.h"
#include "vulkan/shader.h"

using namespace mgp;

ShaderManager::ShaderManager()
	: m_shaderStageCache()
	, m_shaderCache()
{
}

ShaderManager::~ShaderManager()
{
}

void ShaderManager::init(VulkanCore *core)
{
	m_core = core;

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

	ShaderStage *stage = new ShaderStage(m_core, stageType, path.c_str());
	m_shaderStageCache.insert({ name, stage });

	return stage;
}

void ShaderManager::addShader(const std::string &name, Shader *shader)
{
	shader->finalize();
	m_shaderCache.insert({ name, shader });
}

void ShaderManager::loadShaders()
{
	MGP_LOG("Compiling shaders...");

	// shader stages
	{
		// vertex shaders
		loadShaderStage("primitive_vs",						"primitive_vs",							VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("skybox_vs",						"skybox",								VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("fullscreen_triangle_vs",			"fullscreen_triangle_vs",				VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("model_vs",							"model_vs",								VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("model_shadow_map_vs",				"model_shadow_map_vs",					VK_SHADER_STAGE_VERTEX_BIT);

		// fragment shaders
		loadShaderStage("equirectangular_to_cubemap_fs",	"equirectangular_to_cubemap_fs",		VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("irradiance_convolution_fs",		"irradiance_convolution_fs",			VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("prefilter_convolution_fs",			"prefilter_convolution_fs",				VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("brdf_integrator_fs",				"brdf_integrator_fs",					VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("texturedPBR_fs",					"texturedPBR_fs",						VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("texturedPBR_gbuffer_fs",			"texturedPBR_gbuffer_fs",				VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("deferred_lighting_ambient_fs",		"deferred_lighting_ambient_fs",			VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("skybox_fs",						"skybox",								VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("texture_uv_fs",					"texture_uv_fs",						VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("shadow_map_fs",					"shadow_map_fs",						VK_SHADER_STAGE_FRAGMENT_BIT);

		// compute shaders
		loadShaderStage("hdr_tonemapping_cs", "hdr_tonemapping_cs",									VK_SHADER_STAGE_COMPUTE_BIT);
	}

	// effects
	{
		// PBR
		{
			Shader *pbr = new Shader(m_core);
			{
				pbr->setPushConstantsSize(sizeof(int) * 16);
				pbr->setDescriptorSetLayouts({ m_core->getBindlessResources().getLayout() });
				pbr->addStage(getShaderStage("model_vs"));
				pbr->addStage(getShaderStage("texturedPBR_fs"));
			}
			addShader("texturedPBR", pbr);
		}

		// PBR G-BUFFER
		{
			Shader *pbr_gbuffer = new Shader(m_core);
			{
				pbr_gbuffer->setPushConstantsSize(sizeof(int) * 16);
				pbr_gbuffer->setDescriptorSetLayouts({ m_core->getBindlessResources().getLayout() });
				pbr_gbuffer->addStage(getShaderStage("model_vs"));
				pbr_gbuffer->addStage(getShaderStage("texturedPBR_gbuffer_fs"));
			}
			addShader("texturedPBR_gbuffer", pbr_gbuffer);
		}

		// DEFERRED LIGHTING
		{
			Shader *dlighting = new Shader(m_core);
			{
				dlighting->setPushConstantsSize(sizeof(int)*12 + sizeof(float)*4);
				dlighting->setDescriptorSetLayouts({ m_core->getBindlessResources().getLayout() });
				dlighting->addStage(getShaderStage("fullscreen_triangle_vs"));
				dlighting->addStage(getShaderStage("deferred_lighting_ambient_fs"));
			}
			addShader("deferred_lighting_ambient", dlighting);
		}

		// SHADOW MAPPING
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *shadowMap = new Shader(m_core);
			{
				shadowMap->setPushConstantsSize(sizeof(float)*16);
				shadowMap->setDescriptorSetLayouts({ layout });
				shadowMap->addStage(getShaderStage("model_shadow_map_vs"));
				shadowMap->addStage(getShaderStage("shadow_map_fs"));
			}
			addShader("shadow_map", shadowMap);
		}

		// SKYBOX
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *skybox = new Shader(m_core);
			{
				skybox->setPushConstantsSize(sizeof(float)*16 * 2);
				skybox->setDescriptorSetLayouts({ layout });
				skybox->addStage(getShaderStage("skybox_vs"));
				skybox->addStage(getShaderStage("skybox_fs"));
			}
			addShader("skybox", skybox);
		}

		// EQUIRECTANGULAR TO CUBEMAP
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *equirectangularToCubemap = new Shader(m_core);
			{
				equirectangularToCubemap->setPushConstantsSize(sizeof(float)*16 * 2);
				equirectangularToCubemap->setDescriptorSetLayouts({ layout });
				equirectangularToCubemap->addStage(getShaderStage("primitive_vs"));
				equirectangularToCubemap->addStage(getShaderStage("equirectangular_to_cubemap_fs"));
			}
			addShader("equirectangular_to_cubemap", equirectangularToCubemap);
		}

		// IRRADIANCE CONVOLUTION
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *irradianceConvolution = new Shader(m_core);
			{
				irradianceConvolution->setPushConstantsSize(sizeof(float)*16 * 2);
				irradianceConvolution->setDescriptorSetLayouts({ layout });
				irradianceConvolution->addStage(getShaderStage("primitive_vs"));
				irradianceConvolution->addStage(getShaderStage("irradiance_convolution_fs"));
			}
			addShader("irradiance_convolution", irradianceConvolution);
		}

		// PREFILTER CONVOLUTION
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
				.bind(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *prefilterConvolution = new Shader(m_core);
			{
				prefilterConvolution->setPushConstantsSize(sizeof(float)*16 * 2);
				prefilterConvolution->setDescriptorSetLayouts({ layout });
				prefilterConvolution->addStage(getShaderStage("primitive_vs"));
				prefilterConvolution->addStage(getShaderStage("prefilter_convolution_fs"));
			}
			addShader("prefilter_convolution", prefilterConvolution);
		}

		// BRDF LUT GENERATION
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *brdfLUT = new Shader(m_core);
			{
				brdfLUT->setPushConstantsSize(0);
				brdfLUT->setDescriptorSetLayouts({ layout });
				brdfLUT->addStage(getShaderStage("fullscreen_triangle_vs"));
				brdfLUT->addStage(getShaderStage("brdf_integrator_fs"));
			}
			addShader("brdf_lut", brdfLUT);
		}

		// HDR TONEMAPPING
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
				.build(m_core, VK_SHADER_STAGE_COMPUTE_BIT);

			Shader *hdrTonemapping = new Shader(m_core);
			{
				hdrTonemapping->setPushConstantsSize(sizeof(uint32_t)*2 + sizeof(float)*1);
				hdrTonemapping->setDescriptorSetLayouts({ layout });
				hdrTonemapping->addStage(getShaderStage("hdr_tonemapping_cs"));
			}
			addShader("hdr_tonemapping", hdrTonemapping);
		}

		// TEXTURE UV
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *textureUV = new Shader(m_core);
			{
				textureUV->setPushConstantsSize(0);
				textureUV->setDescriptorSetLayouts({ layout });
				textureUV->addStage(getShaderStage("fullscreen_triangle_vs"));
				textureUV->addStage(getShaderStage("texture_uv_fs"));
			}
			addShader("texture_uv", textureUV);
		}
	}
}
