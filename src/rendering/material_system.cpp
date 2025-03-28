#include "material_system.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "math/calc.h"

#include "texture_mgr.h"
#include "shader_mgr.h"
#include "mesh_loader.h"

#include "vulkan/core.h"
#include "vulkan/vertex_format.h"
#include "vulkan/texture_view.h"
#include "vulkan/descriptor_builder.h"

llt::MaterialSystem *llt::g_materialSystem = nullptr;

using namespace llt;

MaterialSystem::MaterialSystem()
	: m_registry()
	, m_descriptorPoolAllocator()
	, m_environmentMap()
	, m_irradianceMap()
	, m_prefilterMap()
	, m_brdfLUT()
	, m_equirectangularToCubemapPipeline()
	, m_irradianceGenerationPipeline()
	, m_prefilterGenerationPipeline()
	, m_brdfIntegrationPipeline()
{
}

MaterialSystem::~MaterialSystem()
{
	m_descriptorPoolAllocator.cleanUp();
	m_registry.cleanUp();
}

void MaterialSystem::init()
{
	m_descriptorPoolAllocator.init(64 * mgc::FRAMES_IN_FLIGHT, {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 					0.5f },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 	4.0f },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 			4.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 			2.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 			2.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,	1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,	1.0f },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 			0.5f }
	});

	//Light lights[16] = {};
	//mem::set(lights, 0, sizeof(Light) * 16);
	//lights[0].position = { 0.0f, 5.0f, 0.0f };
	//lights[0].radius = 0.0f;
	//lights[0].attenuation = 0.0f;
	//lights[0].colour = { 1.0f, 1.0f, 1.0f };
	//lights[0].direction = { 1.0f, -1.0f, -1.0f };
	//lights[0].type = LIGHT_TYPE_SUN;
	//lights[0].direction /= glm::length(lights[0].direction);
}

void MaterialSystem::finalise()
{
	m_registry.loadDefaultTechniques();

	CommandBuffer cmd = CommandBuffer::fromGraphics();

	generateEnvironmentMaps(cmd);
	precomputeBRDF(cmd);
}

// todo: this needs to be moved to a seperate class. IrradianceProbe or something idk
// -> also lots of repeating code.
void MaterialSystem::generateEnvironmentMaps(CommandBuffer &cmd)
{
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

	glm::mat4 captureViews[] = {
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,-1.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),-glm::vec3( 0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),-glm::vec3( 0.0f, 0.0f,-1.0f), glm::vec3(0.0f, 1.0f, 0.0f))
	};

	SubMesh *cubeMesh = g_meshLoader->getCubeMesh();

	// ---

	LLT_LOG("Generating environment map...");

	const int ENVIRONMENT_RESOLUTION = 1024;

	m_environmentMap = g_textureManager->createCubemap(
		"environment_map",
		ENVIRONMENT_RESOLUTION,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		4
	);

	BoundTexture hdrImage(
		g_textureManager->getTexture("environmentHDR"),
		g_textureManager->getSampler("linear")
	);

	struct
	{
		glm::mat4 proj;
		glm::mat4 view;
	}
	pc;

	pc.proj = captureProjection;
	pc.view = glm::identity<glm::mat4>();

	ShaderEffect *equirectangularToCubemapShader = g_shaderManager->getEffect("equirectangular_to_cubemap");

	VkDescriptorSet etcDescriptorSet = m_descriptorPoolAllocator.allocate(equirectangularToCubemapShader->getDescriptorSetLayouts());

	DescriptorWriter()
		.writeCombinedImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, hdrImage.getStandardImageInfo())
		.updateSet(etcDescriptorSet);

	m_equirectangularToCubemapPipeline.setShader(equirectangularToCubemapShader);
	m_equirectangularToCubemapPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_equirectangularToCubemapPipeline.setDepthTest(false);
	m_equirectangularToCubemapPipeline.setDepthWrite(false);

	cmd.beginRecording();
	{
		m_environmentMap->transitionLayout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		for (int i = 0; i < 6; i++)
		{
			pc.view = captureViews[i];

			TextureView view = m_environmentMap->getView(1, i, 0);

			RenderInfo targetInfo;
			targetInfo.setSize(ENVIRONMENT_RESOLUTION, ENVIRONMENT_RESOLUTION);
			targetInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, view);

			PipelineData pipelineData = g_vkCore->getPipelineCache().fetchGraphicsPipeline(m_equirectangularToCubemapPipeline, targetInfo);

			// todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/
			cmd.beginRendering(targetInfo);
			{
				cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

				cmd.bindDescriptorSets(0, pipelineData.layout, { etcDescriptorSet }, {});

				cmd.setViewport({ 0, 0, ENVIRONMENT_RESOLUTION, ENVIRONMENT_RESOLUTION });

				cmd.pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(pc),
					&pc
				);

				cubeMesh->render(cmd);
			}
			cmd.endRendering();
		}

		m_environmentMap->generateMipmaps(cmd);
	}
	cmd.submit();

	// ---
	
	LLT_LOG("Generating irradiance map...");

	const int IRRADIANCE_RESOLUTION = 32;

	m_irradianceMap = g_textureManager->createCubemap(
		"irradiance_map",
		IRRADIANCE_RESOLUTION,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		4
	);

	BoundTexture envMapImage(
		m_environmentMap,
		g_textureManager->getSampler("linear")
	);
	
	ShaderEffect *irradianceGenerationShader = g_shaderManager->getEffect("irradiance_convolution");

	VkDescriptorSet icDescriptorSet = m_descriptorPoolAllocator.allocate(irradianceGenerationShader->getDescriptorSetLayouts());

	DescriptorWriter()
		.writeCombinedImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, envMapImage.getStandardImageInfo())
		.updateSet(icDescriptorSet);

	m_irradianceGenerationPipeline.setShader(irradianceGenerationShader);
	m_irradianceGenerationPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_irradianceGenerationPipeline.setDepthTest(false);
	m_irradianceGenerationPipeline.setDepthWrite(false);

	cmd.beginRecording();
	{
		m_irradianceMap->transitionLayout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		for (int i = 0; i < 6; i++)
		{
			pc.view = captureViews[i];

			TextureView view = m_irradianceMap->getView(1, i, 0);

			RenderInfo targetInfo;
			targetInfo.setSize(IRRADIANCE_RESOLUTION, IRRADIANCE_RESOLUTION);
			targetInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, view);

			PipelineData pipelineData = g_vkCore->getPipelineCache().fetchGraphicsPipeline(m_irradianceGenerationPipeline, targetInfo);

			// todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/
			cmd.beginRendering(targetInfo);
			{
				cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

				cmd.bindDescriptorSets(0, pipelineData.layout, { icDescriptorSet }, {});

				cmd.setViewport({ 0, 0, IRRADIANCE_RESOLUTION, IRRADIANCE_RESOLUTION });

				cmd.pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(pc),
					&pc
				);

				cubeMesh->render(cmd);
			}
			cmd.endRendering();
		}

		m_irradianceMap->generateMipmaps(cmd);
	}
	cmd.submit();

	// ---

	LLT_LOG("Generating prefilter map...");

	const int PREFILTER_RESOLUTION = 128;
	const int PREFILTER_MIP_LEVELS = 5;

	m_prefilterMap = g_textureManager->createCubemap(
		"prefilter_map",
		PREFILTER_RESOLUTION,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		PREFILTER_MIP_LEVELS
	);

	struct
	{
		float roughness = 0.0f;
		float _padding0 = 0.0f;
		float _padding1 = 0.0f;
		float _padding2 = 0.0f;
	}
	prefilterParams;

	GPUBuffer *pfParameterBuffer = g_gpuBufferManager->createUniformBuffer(sizeof(prefilterParams) * PREFILTER_MIP_LEVELS);

	ShaderEffect *prefilterGenerationShader = g_shaderManager->getEffect("prefilter_convolution");

	VkDescriptorSet pfDescriptorSet = m_descriptorPoolAllocator.allocate(prefilterGenerationShader->getDescriptorSetLayouts());

	DescriptorWriter()
		.writeBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, pfParameterBuffer->getDescriptorInfoRange(sizeof(prefilterParams)))
		.writeCombinedImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, envMapImage.getStandardImageInfo())
		.updateSet(pfDescriptorSet);

	m_prefilterGenerationPipeline.setShader(prefilterGenerationShader);
	m_prefilterGenerationPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_prefilterGenerationPipeline.setDepthTest(false);
	m_prefilterGenerationPipeline.setDepthWrite(false);

	cmd.beginRecording();
	{
		m_prefilterMap->transitionLayout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		for (int mipLevel = 0; mipLevel < PREFILTER_MIP_LEVELS; mipLevel++)
		{
			int width = PREFILTER_RESOLUTION >> mipLevel;
			int height = PREFILTER_RESOLUTION >> mipLevel;

			float roughness = (float)mipLevel / (float)(PREFILTER_MIP_LEVELS - 1);

			prefilterParams.roughness = roughness;
			uint32_t dynamicOffset = sizeof(prefilterParams) * mipLevel;
			pfParameterBuffer->writeDataToMe(&prefilterParams, sizeof(prefilterParams), dynamicOffset);

			for (int i = 0; i < 6; i++)
			{
				pc.view = captureViews[i];

				TextureView view = m_prefilterMap->getView(1, i, mipLevel);

				RenderInfo info;
				info.setSize(width, height);
				info.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, view);

				PipelineData pipelineData = g_vkCore->getPipelineCache().fetchGraphicsPipeline(m_prefilterGenerationPipeline, info);

				// todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/
				cmd.beginRendering(info);
				{
					cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

					cmd.bindDescriptorSets(
						0,
						pipelineData.layout,
						{ pfDescriptorSet },
						{ dynamicOffset }
					);

					cmd.setViewport({ 0, 0, (float)width, (float)height });

					cmd.pushConstants(
						pipelineData.layout,
						VK_SHADER_STAGE_ALL_GRAPHICS,
						sizeof(pc),
						&pc
					);

					cubeMesh->render(cmd);
				}
				cmd.endRendering();
			}
		}
	}
	cmd.submit();

	m_prefilterMap->transitionLayoutSingle(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	delete pfParameterBuffer;
}

void MaterialSystem::precomputeBRDF(CommandBuffer &cmd)
{
	LLT_LOG("Precomputing BRDF...");

	SubMesh *quadMesh = g_meshLoader->getQuadMesh();

	const int BRDF_RESOLUTION = 512;

	m_brdfLUT = g_textureManager->createAttachment(
		"brdfIntegration",
		BRDF_RESOLUTION, BRDF_RESOLUTION,
		VK_FORMAT_R32G32_SFLOAT,
		VK_IMAGE_TILING_LINEAR
	);

	RenderInfo info;
	info.setSize(BRDF_RESOLUTION, BRDF_RESOLUTION);
	info.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, m_brdfLUT->getStandardView());

	ShaderEffect *brdfLUTShader = g_shaderManager->getEffect("brdf_lut");

	VkDescriptorSet set = m_descriptorPoolAllocator.allocate(brdfLUTShader->getDescriptorSetLayouts());

	m_brdfIntegrationPipeline.setShader(brdfLUTShader);
	m_brdfIntegrationPipeline.setVertexFormat(g_primitiveUvVertexFormat);
	m_brdfIntegrationPipeline.setDepthTest(false);
	m_brdfIntegrationPipeline.setDepthWrite(false);

	PipelineData pipelineData = g_vkCore->getPipelineCache().fetchGraphicsPipeline(m_brdfIntegrationPipeline, info);

	cmd.beginRecording();
	cmd.beginRendering(info);

	cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

	cmd.bindDescriptorSets(0, pipelineData.layout, { set }, {});

	cmd.setViewport({ 0, 0, BRDF_RESOLUTION, BRDF_RESOLUTION });

	quadMesh->render(cmd);

	cmd.endRendering();
	cmd.submit();
}

MaterialRegistry &MaterialSystem::getRegistry()
{
	return m_registry;
}

const MaterialRegistry &MaterialSystem::getRegistry() const
{
	return m_registry;
}

Texture *MaterialSystem::getIrradianceMap() const
{
	return m_irradianceMap;
}

Texture *MaterialSystem::getPrefilterMap() const
{
	return m_prefilterMap;
}

Texture *MaterialSystem::getBRDFLUT() const
{
	return m_brdfLUT;
}

Texture *MaterialSystem::getDiffuseFallback() const
{
	return g_textureManager->getTexture("fallback_white");
}

Texture *MaterialSystem::getAOFallback() const
{
	return g_textureManager->getTexture("fallback_white");
}

Texture *MaterialSystem::getRoughnessMetallicFallback() const
{
	return g_textureManager->getTexture("fallback_black");
}

Texture *MaterialSystem::getNormalFallback() const
{
	return g_textureManager->getTexture("fallback_normals");
}

Texture *MaterialSystem::getEmissiveFallback() const
{
	return g_textureManager->getTexture("fallback_black");
}

// ---

void MaterialRegistry::cleanUp()
{
	for (auto &[id, mat] : m_materials) {
		delete mat;
	}

	m_materials.clear();
	m_techniques.clear();
}

void MaterialRegistry::loadDefaultTechniques()
{
	// PBR
	{
		Technique pbrTechnique;
		pbrTechnique.passes[SHADER_PASS_FORWARD] = g_shaderManager->getEffect("texturedPBR");
		pbrTechnique.passes[SHADER_PASS_SHADOW] = nullptr;
		pbrTechnique.vertexFormat = g_modelVertexFormat;
		addTechnique("texturedPBR_opaque", pbrTechnique);
	}

	// SUBSURFACE REFRACTION
	{
		/*
		Technique subsurfaceRefractionTechnique;
		subsurfaceRefractionTechnique.passes[SHADER_PASS_FORWARD] = g_shaderManager->getEffect("subsurface_refraction");
		subsurfaceRefractionTechnique.passes[SHADER_PASS_SHADOW] = nullptr;
		subsurfaceRefractionTechnique.vertexFormat = g_modelVertexFormat;
		addTechnique("subsurface_refraction", subsurfaceRefractionTechnique);
		*/
	}
}

Material *MaterialRegistry::buildMaterial(MaterialData &data)
{
	if (m_materials.contains(data.getHash()))
		return m_materials.get(data.getHash());

	cauto &technique = m_techniques.get(data.technique);

	Material *material = new Material();
	material->m_vertexFormat = technique.vertexFormat;
	material->m_textures = data.textures;

	/*
	material->m_parameterBuffer = g_shaderBufferManager->createDynamicUBO(sizeof(float) * 32);

	if (data.parameterSize > 0)
	{
	material->m_parameterBuffer->pushData(
	data.parameters,
	data.parameterSize
	);
	}
	else
	{
	struct
	{
	int asdf = 0;
	}
	garbageDefaultParametersWhatIsEvenGoingOn;

	material->m_parameterBuffer->pushData(
	&garbageDefaultParametersWhatIsEvenGoingOn,
	sizeof(garbageDefaultParametersWhatIsEvenGoingOn)
	);
	}
	*/

	for (int i = 0; i < SHADER_PASS_MAX_ENUM; i++)
	{
		if (!technique.passes[i])
			continue;

		material->m_passes[i].shader = technique.passes[i];

		material->m_passes[i].pipeline.setShader(technique.passes[i]);
		material->m_passes[i].pipeline.setVertexFormat(technique.vertexFormat);
		material->m_passes[i].pipeline.setDepthTest(technique.depthTest);
		material->m_passes[i].pipeline.setDepthWrite(technique.depthWrite);
	}

	m_materials.insert(data.getHash(), material);

	return material;
}

void MaterialRegistry::addTechnique(const String &name, const Technique &technique)
{
	m_techniques.insert(name, technique);
}
