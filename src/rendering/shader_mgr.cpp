#include "shader_mgr.h"

#include "vulkan/core.h"

#include "io/file_stream.h"

#include <fstream>
#include <vector>

llt::ShaderMgr *llt::g_shaderManager = nullptr;

using namespace llt;

ShaderMgr::ShaderMgr()
	: m_shaderCache()
	, m_effects()
{
}

ShaderMgr::~ShaderMgr()
{
	for (auto &[name, effect] : m_effects)
		delete effect;

	for (auto &[name, shader] : m_shaderCache)
		delete shader;

	m_effects.clear();
	m_shaderCache.clear();
}

void ShaderMgr::loadDefaultShaders()
{
	loadDefaultShaderPrograms();
	createDefaultShaderEffects();
}

void ShaderMgr::loadDefaultShaderPrograms()
{
	load("primitive_vs",					"../../res/shaders/compiled/primitive_vs.spv",						VK_SHADER_STAGE_VERTEX_BIT);
	load("skybox_vs",						"../../res/shaders/compiled/skybox_vs.spv",							VK_SHADER_STAGE_VERTEX_BIT);
	load("primitive_quad_vs",				"../../res/shaders/compiled/primitive_quad_vs.spv",					VK_SHADER_STAGE_VERTEX_BIT);
	load("model_vs",						"../../res/shaders/compiled/model_vs.spv",							VK_SHADER_STAGE_VERTEX_BIT);

	load("equirectangular_to_cubemap_ps",	"../../res/shaders/compiled/equirectangular_to_cubemap_ps.spv",		VK_SHADER_STAGE_FRAGMENT_BIT);
	load("irradiance_convolution_ps",		"../../res/shaders/compiled/irradiance_convolution_ps.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
	load("prefilter_convolution_ps",		"../../res/shaders/compiled/prefilter_convolution_ps.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
	load("brdf_integrator_ps",				"../../res/shaders/compiled/brdf_integrator_ps.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
	load("texturedPBR_ps",					"../../res/shaders/compiled/texturedPBR_ps.spv",					VK_SHADER_STAGE_FRAGMENT_BIT);
	load("skybox_ps",						"../../res/shaders/compiled/skybox_ps.spv",							VK_SHADER_STAGE_FRAGMENT_BIT);
	load("hdr_tonemapping_ps",				"../../res/shaders/compiled/hdr_tonemapping_ps.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
	load("bloom_downsample_ps",				"../../res/shaders/compiled/bloom_downsample_ps.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
	load("bloom_upsample_ps",				"../../res/shaders/compiled/bloom_upsample_ps.spv",					VK_SHADER_STAGE_FRAGMENT_BIT);
}

void ShaderMgr::createDefaultShaderEffects()
{
	// PBR
	{
		DescriptorLayoutBuilder descriptorLayoutBuilder;
		
		// bind all buffers
		for (int i = 0; i <= 2; i++)
			descriptorLayoutBuilder.bind(i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

		// bind all textures
		for (int i = 3; i <= 10; i++)
			descriptorLayoutBuilder.bind(i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		VkDescriptorSetLayout layout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS);

		ShaderEffect *pbr_effect = createEffect("texturedPBR");
		pbr_effect->setDescriptorSetLayout(layout);
		pbr_effect->setPushConstantsSize(0);
		pbr_effect->addStage(g_shaderManager->get("model_vs"));
		pbr_effect->addStage(g_shaderManager->get("texturedPBR_ps"));
	}

	// SKYBOX
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(VK_SHADER_STAGE_ALL_GRAPHICS);

		ShaderEffect *skybox_effect = createEffect("skybox");
		skybox_effect->setDescriptorSetLayout(layout);
		skybox_effect->setPushConstantsSize(sizeof(float)*16 * 2);
		skybox_effect->addStage(get("skybox_vs"));
		skybox_effect->addStage(get("skybox_ps"));
	}

	// EQUIRECTANGULAR TO CUBEMAP
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(VK_SHADER_STAGE_ALL_GRAPHICS);

		ShaderEffect *equirectangularToCubemap_effect = createEffect("equirectangular_to_cubemap");
		equirectangularToCubemap_effect->setDescriptorSetLayout(layout);
		equirectangularToCubemap_effect->setPushConstantsSize(sizeof(float)*16 * 2);
		equirectangularToCubemap_effect->addStage(get("primitive_vs"));
		equirectangularToCubemap_effect->addStage(get("equirectangular_to_cubemap_ps"));
	}

	// IRRADIANCE CONVOLUTION
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(VK_SHADER_STAGE_ALL_GRAPHICS);

		ShaderEffect *irradianceConvolution_effect = createEffect("irradiance_convolution");
		irradianceConvolution_effect->setDescriptorSetLayout(layout);
		irradianceConvolution_effect->setPushConstantsSize(sizeof(float)*16 * 2);
		irradianceConvolution_effect->addStage(get("primitive_vs"));
		irradianceConvolution_effect->addStage(get("irradiance_convolution_ps"));
	}

	// PREFILTER CONVOLUTION
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.bind(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
			.bind(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(VK_SHADER_STAGE_ALL_GRAPHICS);

		ShaderEffect *prefilterConvolution_effect = createEffect("prefilter_convolution");
		prefilterConvolution_effect->setDescriptorSetLayout(layout);
		prefilterConvolution_effect->setPushConstantsSize(sizeof(float)*16 * 2);
		prefilterConvolution_effect->addStage(get("primitive_vs"));
		prefilterConvolution_effect->addStage(get("prefilter_convolution_ps"));
	}

	// BRDF LUT GENERATION
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.build(VK_SHADER_STAGE_ALL_GRAPHICS);

		ShaderEffect *brdfLUT_effect = createEffect("brdf_lut");
		brdfLUT_effect->setDescriptorSetLayout(layout);
		brdfLUT_effect->setPushConstantsSize(0);
		brdfLUT_effect->addStage(get("primitive_quad_vs"));
		brdfLUT_effect->addStage(get("brdf_integrator_ps"));
	}

	// HDR TONEMAPPING
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.bind(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(VK_SHADER_STAGE_ALL_GRAPHICS);

		ShaderEffect *hdrTonemapping_effect = createEffect("hdr_tonemapping");
		hdrTonemapping_effect->setDescriptorSetLayout(layout);
		hdrTonemapping_effect->setPushConstantsSize(sizeof(float)*2);
		hdrTonemapping_effect->addStage(get("primitive_quad_vs"));
		hdrTonemapping_effect->addStage(get("hdr_tonemapping_ps"));
	}

	// BLOOM DOWNSAMPLE
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(VK_SHADER_STAGE_ALL_GRAPHICS);

		ShaderEffect *bloomDownsample_effect = createEffect("bloom_downsample");
		bloomDownsample_effect->setDescriptorSetLayout(layout);
		bloomDownsample_effect->setPushConstantsSize(sizeof(int));
		bloomDownsample_effect->addStage(get("primitive_quad_vs"));
		bloomDownsample_effect->addStage(get("bloom_downsample_ps"));
	}

	// BLOOM UPSAMPLE
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(VK_SHADER_STAGE_ALL_GRAPHICS);

		ShaderEffect *bloomUpsample_effect = createEffect("bloom_upsample");
		bloomUpsample_effect->setDescriptorSetLayout(layout);
		bloomUpsample_effect->setPushConstantsSize(sizeof(float));
		bloomUpsample_effect->addStage(get("primitive_quad_vs"));
		bloomUpsample_effect->addStage(get("bloom_upsample_ps"));
	}
}

ShaderProgram *ShaderMgr::get(const String &name)
{
	return m_shaderCache.getOrDefault(name, nullptr);
}

ShaderProgram *ShaderMgr::load(const String &name, const String &source, VkShaderStageFlagBits stage)
{
	if (m_shaderCache.contains(name))
		return m_shaderCache.get(name);

	/*
	FileStream fs(source.cstr(), "r");
	Vector<char> sourceData(fs.size());
	fs.read(sourceData.data(), fs.size());
	fs.close();
	*/

	// todo: there should be platform abstractions for the file handles
	// like FileStream myFile = g_platform->openFile(...)
	// on windows it uses the SDL i/o streams
	// on mac / linux something else idk

	std::ifstream file(source.cstr(), std::ios::binary | std::ios::ate);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> sourceData(size);
	file.read(sourceData.data(), size);

	ShaderProgram *shader = new ShaderProgram();
	shader->setStage(stage);
	shader->loadFromSource(sourceData.data(), sourceData.size());

	m_shaderCache.insert(name, shader);

	return shader;
}

ShaderEffect *ShaderMgr::getEffect(const String &name)
{
	return m_effects.getOrDefault(name, nullptr);
}

ShaderEffect *ShaderMgr::createEffect(const String &name)
{
	if (m_effects.contains(name))
		return m_effects.get(name);

	ShaderEffect *effect = new ShaderEffect();
	
	m_effects.insert(name, effect);
	
	return effect;
}
