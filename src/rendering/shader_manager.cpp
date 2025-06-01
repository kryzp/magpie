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
	return m_shaderStageCache.at(name);
}

Shader *ShaderManager::getShader(const std::string &name)
{
	return m_shaderCache.at(name);
}

ShaderStage *ShaderManager::loadShaderStage(const std::string &name, const std::string &path, VkShaderStageFlagBits stageType)
{
	if (m_shaderStageCache.contains(name))
		return m_shaderStageCache.at(name);

	ShaderStage *stage = new ShaderStage(m_core, stageType, path.c_str());
	m_shaderStageCache.insert({ name, stage });

	return stage;
}

Shader *ShaderManager::createShader(const std::string &name)
{
	if (m_shaderCache.contains(name))
		return m_shaderCache.at(name);

	Shader *s = new Shader();
	m_shaderCache.insert({ name, s });
	
	return s;
}

void ShaderManager::loadShaders()
{
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
		loadShaderStage("skybox_fs",						"skybox",								VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("texture_uv_fs",					"texture_uv_fs",						VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("shadow_map_fs",					"shadow_map_fs",						VK_SHADER_STAGE_FRAGMENT_BIT);

		// compute shaders
		loadShaderStage("hdr_tonemapping_cs", "hdr_tonemapping_cs",									VK_SHADER_STAGE_COMPUTE_BIT);
		
		/*
		loadShaderStage("primitive_vs",						"../../res/shaders/compiled/primitive_vs.spv",						VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("skybox_vs",						"../../res/shaders/compiled/skybox_vs.spv",							VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("fullscreen_triangle_vs",			"../../res/shaders/compiled/fullscreen_triangle_vs.spv",			VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("model_vs",							"../../res/shaders/compiled/model_vs.spv",							VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("model_shadow_map_vs",				"../../res/shaders/compiled/model_shadow_map_vs.spv",				VK_SHADER_STAGE_VERTEX_BIT);

		loadShaderStage("equirectangular_to_cubemap_fs",	"../../res/shaders/compiled/equirectangular_to_cubemap_ps.spv",		VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("irradiance_convolution_fs",		"../../res/shaders/compiled/irradiance_convolution_ps.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("prefilter_convolution_fs",			"../../res/shaders/compiled/prefilter_convolution_ps.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("brdf_integrator_fs",				"../../res/shaders/compiled/brdf_integrator_ps.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("texturedPBR_fs",					"../../res/shaders/compiled/texturedPBR_ps.spv",					VK_SHADER_STAGE_FRAGMENT_BIT);
//		loadShaderStage("subsurface_refraction_fs",			"../../res/shaders/compiled/subsurface_refraction_ps.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("skybox_fs",						"../../res/shaders/compiled/skybox_ps.spv",							VK_SHADER_STAGE_FRAGMENT_BIT);
//		loadShaderStage("bloom_downsample_fs",				"../../res/shaders/compiled/bloom_downsample_ps.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
//		loadShaderStage("bloom_upsample_fs",				"../../res/shaders/compiled/bloom_upsample_ps.spv",					VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("texture_uv_fs",					"../../res/shaders/compiled/texture_uv_ps.spv",						VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("shadow_map_fs",					"../../res/shaders/compiled/shadow_map_ps.spv",						VK_SHADER_STAGE_FRAGMENT_BIT);

		loadShaderStage("hdr_tonemapping_cs",				"../../res/shaders/compiled/hdr_tonemapping_cs.spv",				VK_SHADER_STAGE_COMPUTE_BIT);
		*/

		MGP_LOG("Compiled all shaders.");
	}

	// effects
	{
		// PBR
		{
			Shader *pbr = createShader("texturedPBR");

			pbr->setDescriptorSetLayouts({ m_core->getBindlessResources().getLayout()});
			pbr->setPushConstantsSize(sizeof(int) * 16);
			pbr->addStage(getShaderStage("model_vs"));
			pbr->addStage(getShaderStage("texturedPBR_fs"));
		}

		// SHADOW MAPPING
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *shadowMap = createShader("shadow_map");

			shadowMap->setDescriptorSetLayouts({ layout });
			shadowMap->setPushConstantsSize(sizeof(float)*16);
			shadowMap->addStage(getShaderStage("model_shadow_map_vs"));
			shadowMap->addStage(getShaderStage("shadow_map_fs"));
		}

		// SKYBOX
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *skybox = createShader("skybox");

			skybox->setDescriptorSetLayouts({ layout });
			skybox->setPushConstantsSize(sizeof(float)*16 * 2);
			skybox->addStage(getShaderStage("skybox_vs"));
			skybox->addStage(getShaderStage("skybox_fs"));
		}

		// EQUIRECTANGULAR TO CUBEMAP
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *equirectangularToCubemap = createShader("equirectangular_to_cubemap");

			equirectangularToCubemap->setDescriptorSetLayouts({ layout });
			equirectangularToCubemap->setPushConstantsSize(sizeof(float)*16 * 2);
			equirectangularToCubemap->addStage(getShaderStage("primitive_vs"));
			equirectangularToCubemap->addStage(getShaderStage("equirectangular_to_cubemap_fs"));
		}

		// IRRADIANCE CONVOLUTION
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *irradianceConvolution = createShader("irradiance_convolution");

			irradianceConvolution->setDescriptorSetLayouts({ layout });
			irradianceConvolution->setPushConstantsSize(sizeof(float)*16 * 2);
			irradianceConvolution->addStage(getShaderStage("primitive_vs"));
			irradianceConvolution->addStage(getShaderStage("irradiance_convolution_fs"));
		}

		// PREFILTER CONVOLUTION
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
				.bind(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *prefilterConvolution = createShader("prefilter_convolution");

			prefilterConvolution->setDescriptorSetLayouts({ layout });
			prefilterConvolution->setPushConstantsSize(sizeof(float)*16 * 2);
			prefilterConvolution->addStage(getShaderStage("primitive_vs"));
			prefilterConvolution->addStage(getShaderStage("prefilter_convolution_fs"));
		}

		// BRDF LUT GENERATION
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *brdfLUT = createShader("brdf_lut");

			brdfLUT->setDescriptorSetLayouts({ layout });
			brdfLUT->setPushConstantsSize(0);
			brdfLUT->addStage(getShaderStage("fullscreen_triangle_vs"));
			brdfLUT->addStage(getShaderStage("brdf_integrator_fs"));
		}

		// HDR TONEMAPPING
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
				.build(m_core, VK_SHADER_STAGE_COMPUTE_BIT);

			Shader *hdrTonemapping = createShader("hdr_tonemapping");

			hdrTonemapping->setDescriptorSetLayouts({ layout });
			hdrTonemapping->setPushConstantsSize(sizeof(uint32_t)*2 + sizeof(float)*1);
			hdrTonemapping->addStage(getShaderStage("hdr_tonemapping_cs"));
		}

		// TEXTURE UV
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *textureUV = createShader("texture_uv");

			textureUV->setDescriptorSetLayouts({ layout });
			textureUV->setPushConstantsSize(0);
			textureUV->addStage(getShaderStage("fullscreen_triangle_vs"));
			textureUV->addStage(getShaderStage("texture_uv_fs"));
		}
	}
}
