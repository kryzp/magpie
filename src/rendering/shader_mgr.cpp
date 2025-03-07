#include "shader_mgr.h"

#include <fstream>
#include <sstream>
#include <string>

#include "vulkan/backend.h"
#include "io/file_stream.h"

llt::ShaderMgr *llt::g_shaderManager = nullptr;

using namespace llt;

ShaderMgr::ShaderMgr()
	: m_shaderCache()
	, m_effects()
	, m_descriptorLayoutCache()
{
}

ShaderMgr::~ShaderMgr()
{
	m_descriptorLayoutCache.cleanUp();

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
	load("primitiveVS",					"../../res/shaders/raster/primitive_vs.spv",				VK_SHADER_STAGE_VERTEX_BIT);
	load("primitiveQuadVS",				"../../res/shaders/raster/primitive_quad_vs.spv",			VK_SHADER_STAGE_VERTEX_BIT);
	load("genericVertex",				"../../res/shaders/raster/model_vs.spv",					VK_SHADER_STAGE_VERTEX_BIT);

	load("equirectangularToCubemap",	"../../res/shaders/raster/equirectangular_to_cubemap.spv",	VK_SHADER_STAGE_FRAGMENT_BIT);
	load("irradianceConvolution",		"../../res/shaders/raster/irradiance_convolution.spv",		VK_SHADER_STAGE_FRAGMENT_BIT);
	load("prefilterConvolution",		"../../res/shaders/raster/prefilter_convolution.spv",		VK_SHADER_STAGE_FRAGMENT_BIT);
	load("brdfIntegrator",				"../../res/shaders/raster/brdf_integrator.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
	load("pbrFragment",					"../../res/shaders/raster/texturedPBR.spv",					VK_SHADER_STAGE_FRAGMENT_BIT);
	load("skyboxFragment",				"../../res/shaders/raster/skybox.spv",						VK_SHADER_STAGE_FRAGMENT_BIT);
	load("hdrTonemappingPS",			"../../res/shaders/raster/hdr_tonemapping.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
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

		VkDescriptorSetLayout layout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, &m_descriptorLayoutCache);

		ShaderEffect *pbr_effect = createEffect("texturedPBR_opaque");
		pbr_effect->setDescriptorSetLayout(layout);
		pbr_effect->setPushConstantsSize(0);
		pbr_effect->addStage(g_shaderManager->get("genericVertex"));
		pbr_effect->addStage(g_shaderManager->get("pbrFragment"));
	}

	// SKYBOX
	{
		DescriptorLayoutBuilder descriptorLayoutBuilder;
		descriptorLayoutBuilder.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		VkDescriptorSetLayout layout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, &m_descriptorLayoutCache);

		ShaderEffect *skybox_effect = createEffect("skybox");
		skybox_effect->setDescriptorSetLayout(layout);
		skybox_effect->setPushConstantsSize(sizeof(float)*16 * 2);
		skybox_effect->addStage(get("primitiveVS"));
		skybox_effect->addStage(get("skyboxFragment"));
	}

	// EQUIRECTANGULAR TO CUBEMAP
	{
		DescriptorLayoutBuilder descriptorLayoutBuilder;
		descriptorLayoutBuilder.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		VkDescriptorSetLayout layout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, &m_descriptorLayoutCache);

		ShaderEffect *equirectangularToCubemap_effect = createEffect("equirectangularToCubemap");
		equirectangularToCubemap_effect->setDescriptorSetLayout(layout);
		equirectangularToCubemap_effect->setPushConstantsSize(sizeof(float)*16 * 2);
		equirectangularToCubemap_effect->addStage(get("primitiveVS"));
		equirectangularToCubemap_effect->addStage(get("equirectangularToCubemap"));
	}

	// IRRADIANCE CONVOLUTION
	{
		DescriptorLayoutBuilder descriptorLayoutBuilder;
		descriptorLayoutBuilder.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		VkDescriptorSetLayout layout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, &m_descriptorLayoutCache);

		ShaderEffect *irradianceConvolution_effect = createEffect("irradianceConvolution");
		irradianceConvolution_effect->setDescriptorSetLayout(layout);
		irradianceConvolution_effect->setPushConstantsSize(sizeof(float)*16 * 2);
		irradianceConvolution_effect->addStage(get("primitiveVS"));
		irradianceConvolution_effect->addStage(get("irradianceConvolution"));
	}

	// PREFILTER CONVOLUTION
	{
		DescriptorLayoutBuilder descriptorLayoutBuilder;
		descriptorLayoutBuilder.bind(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
		descriptorLayoutBuilder.bind(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		VkDescriptorSetLayout layout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, &m_descriptorLayoutCache);

		ShaderEffect *prefilterConvolution_effect = createEffect("prefilterConvolution");
		prefilterConvolution_effect->setDescriptorSetLayout(layout);
		prefilterConvolution_effect->setPushConstantsSize(sizeof(float)*16 * 2);
		prefilterConvolution_effect->addStage(get("primitiveVS"));
		prefilterConvolution_effect->addStage(get("prefilterConvolution"));
	}

	// BRDF LUT GENERATION
	{
		DescriptorLayoutBuilder descriptorLayoutBuilder;

		VkDescriptorSetLayout layout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, &m_descriptorLayoutCache);

		ShaderEffect *brdfLUT_effect = createEffect("brdfLUT");
		brdfLUT_effect->setDescriptorSetLayout(layout);
		brdfLUT_effect->setPushConstantsSize(0);
		brdfLUT_effect->addStage(get("primitiveQuadVS"));
		brdfLUT_effect->addStage(get("brdfIntegrator"));
	}

	// HDR TONEMAPPING
	{
		DescriptorLayoutBuilder descriptorLayoutBuilder;
		descriptorLayoutBuilder.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		VkDescriptorSetLayout layout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, &m_descriptorLayoutCache);

		ShaderEffect *hdrTonemapping_effect = createEffect("hdrTonemapping");
		hdrTonemapping_effect->setDescriptorSetLayout(layout);
		hdrTonemapping_effect->setPushConstantsSize(0);
		hdrTonemapping_effect->addStage(get("primitiveQuadVS"));
		hdrTonemapping_effect->addStage(get("hdrTonemappingPS"));
	}
}

ShaderProgram *ShaderMgr::get(const String &name)
{
	if (m_shaderCache.contains(name))
		return m_shaderCache.get(name);

	return nullptr;
}

ShaderProgram *ShaderMgr::load(const String &name, const String &source, VkShaderStageFlagBits stage)
{
	if (m_shaderCache.contains(name))
		return m_shaderCache.get(name);

	FileStream fs(source.cstr(), "r");
	Vector<char> sourceData(fs.size());
	fs.read(sourceData.data(), fs.size());
	fs.close();

	ShaderProgram *shader = new ShaderProgram();
	shader->setStage(stage);
	shader->loadFromSource(sourceData.data(), sourceData.size());

	m_shaderCache.insert(name, shader);

	return shader;
}

ShaderEffect *ShaderMgr::getEffect(const String& name)
{
	if (m_effects.contains(name))
		return m_effects.get(name);

	return nullptr;
}

ShaderEffect *ShaderMgr::createEffect(const String &name)
{
	if (m_effects.contains(name))
		return m_effects.get(name);

	ShaderEffect *effect = new ShaderEffect();
	
	m_effects.insert(name, effect);
	
	return effect;
}
