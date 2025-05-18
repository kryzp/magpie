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

	std::ifstream file(path, std::ios::binary | std::ios::ate);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> source(size);
	file.read(source.data(), size);

	ShaderStage *stage = new ShaderStage(m_core, stageType, source.data(), source.size());

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
		loadShaderStage("primitive_vs",						"../../res/shaders/compiled/primitive_vs.spv",						VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("skybox_vs",						"../../res/shaders/compiled/skybox_vs.spv",							VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("fullscreen_triangle_vs",			"../../res/shaders/compiled/fullscreen_triangle_vs.spv",			VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("model_vs",							"../../res/shaders/compiled/model_vs.spv",							VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("model_shadow_map_vs",				"../../res/shaders/compiled/model_shadow_map_vs.spv",				VK_SHADER_STAGE_VERTEX_BIT);

		// pixel shaders
		loadShaderStage("equirectangular_to_cubemap_ps",	"../../res/shaders/compiled/equirectangular_to_cubemap_ps.spv",		VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("irradiance_convolution_ps",		"../../res/shaders/compiled/irradiance_convolution_ps.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("prefilter_convolution_ps",			"../../res/shaders/compiled/prefilter_convolution_ps.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("brdf_integrator_ps",				"../../res/shaders/compiled/brdf_integrator_ps.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("texturedPBR_ps",					"../../res/shaders/compiled/texturedPBR_ps.spv",					VK_SHADER_STAGE_FRAGMENT_BIT);
//		loadShaderStage("subsurface_refraction_ps",			"../../res/shaders/compiled/subsurface_refraction_ps.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("skybox_ps",						"../../res/shaders/compiled/skybox_ps.spv",							VK_SHADER_STAGE_FRAGMENT_BIT);
//		loadShaderStage("bloom_downsample_ps",				"../../res/shaders/compiled/bloom_downsample_ps.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
//		loadShaderStage("bloom_upsample_ps",				"../../res/shaders/compiled/bloom_upsample_ps.spv",					VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("texture_uv_ps",					"../../res/shaders/compiled/texture_uv_ps.spv",						VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("shadow_map_ps",					"../../res/shaders/compiled/shadow_map_ps.spv",						VK_SHADER_STAGE_FRAGMENT_BIT);

		// compute shaders
		loadShaderStage("hdr_tonemapping_cs",				"../../res/shaders/compiled/hdr_tonemapping_cs.spv",				VK_SHADER_STAGE_COMPUTE_BIT);
	}

	// effects
	{
		// PBR
		{
			Shader *pbr = createShader("texturedPBR");

			pbr->setDescriptorSetLayouts({ m_core->getBindlessResources().getLayout()});
			pbr->setPushConstantsSize(sizeof(int) * 16);
			pbr->addStage(getShaderStage("model_vs"));
			pbr->addStage(getShaderStage("texturedPBR_ps"));
		}

		// SUBSURFACE REFRACTION
		/*
		{
			DescriptorLayoutBuilder descriptorLayoutBuilder;

			// bind all buffers
			for (int i = 0; i <= 2; i++)
			descriptorLayoutBuilder.bind(i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

			// bind all textures
			for (int i = 3; i <= 10; i++)
			descriptorLayoutBuilder.bind(i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			VkDescriptorSetLayout layout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *pbr = createEffect("subsurface_refraction");
			pbr->setDescriptorSetLayout(layout);
			pbr->setPushConstantsSize(0);
			pbr->addStage(g_shaderManager->get("model_vs"));
			pbr->addStage(g_shaderManager->get("subsurface_refraction_ps"));
		}
		*/

		// SHADOW MAPPING
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *shadowMap = createShader("shadow_map");

			shadowMap->setDescriptorSetLayouts({ layout });
			shadowMap->setPushConstantsSize(sizeof(float)*16);
			shadowMap->addStage(getShaderStage("model_shadow_map_vs"));
			shadowMap->addStage(getShaderStage("shadow_map_ps"));
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
			skybox->addStage(getShaderStage("skybox_ps"));
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
			equirectangularToCubemap->addStage(getShaderStage("equirectangular_to_cubemap_ps"));
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
			irradianceConvolution->addStage(getShaderStage("irradiance_convolution_ps"));
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
			prefilterConvolution->addStage(getShaderStage("prefilter_convolution_ps"));
		}

		// BRDF LUT GENERATION
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *brdfLUT = createShader("brdf_lut");

			brdfLUT->setDescriptorSetLayouts({ layout });
			brdfLUT->setPushConstantsSize(0);
			brdfLUT->addStage(getShaderStage("fullscreen_triangle_vs"));
			brdfLUT->addStage(getShaderStage("brdf_integrator_ps"));
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

		// BLOOM DOWNSAMPLE
		/*
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_vulkanCore, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *bloomDownsample = createShader("bloom_downsample");

			bloomDownsample->setDescriptorSetLayouts({ layout });
			bloomDownsample->setPushConstantsSize(sizeof(int));
			bloomDownsample->addStage(getShaderStage("fullscreen_triangle_vs"));
			bloomDownsample->addStage(getShaderStage("bloom_downsample_ps"));
		}

		// BLOOM UPSAMPLE
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_vulkanCore, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *bloomUpsample = createShader("bloom_upsample");

			bloomUpsample->setDescriptorSetLayouts({ layout });
			bloomUpsample->setPushConstantsSize(sizeof(float));
			bloomUpsample->addStage(getShaderStage("fullscreen_triangle_vs"));
			bloomUpsample->addStage(getShaderStage("bloom_upsample_ps"));
		}
		*/

		// TEXTURE UV
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *textureUV = createShader("texture_uv");

			textureUV->setDescriptorSetLayouts({ layout });
			textureUV->setPushConstantsSize(0);
			textureUV->addStage(getShaderStage("fullscreen_triangle_vs"));
			textureUV->addStage(getShaderStage("texture_uv_ps"));
		}
	}
}
