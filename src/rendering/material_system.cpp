#include "material_system.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "shader_mgr.h"
#include "mesh_loader.h"

#include "vulkan/vertex_format.h"
#include "vulkan/backend.h"

#include "math/calc.h"

#include "../../res/shaders/raster/common_fxc.inc"

llt::MaterialSystem *llt::g_materialSystem = nullptr;

using namespace llt;

/*
enum class TextureHandle : uint32_t { INVALID = Calc<uint32_t>::maxValue() };
enum class BufferHandle : uint32_t { INVALID = Calc<uint32_t>::maxValue() };

struct Params_PBR {
	BufferHandle meshTransforms;
	BufferHandle pointLights;
	BufferHandle camera;
	uint32_t _padding0;
};

struct Params_Skybox {
	BufferHandle camera;
	TextureHandle skybox;
	uint32_t _padding0;
	uint32_t _padding1;
};
*/

MaterialSystem::MaterialSystem()
	: m_materials()
	, m_techniques()
	, m_globalBuffer()
	, m_instanceBuffer()
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

	for (auto& [id, mat] : m_materials) {
		delete mat;
	}

	m_materials.clear();
	m_techniques.clear();
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

	m_globalBuffer = g_shaderBufferManager->createUBO();
	m_instanceBuffer = g_shaderBufferManager->createUBO();

	Light lights[16] = {};
	mem::set(lights, 0, sizeof(Light) * 16);

	lights[0].position = { 0.0f, 5.0f, 0.0f };
	lights[0].radius = 0.0f;
	lights[0].attenuation = 0.0f;
	lights[0].colour = { 1.0f, 1.0f, 1.0f };
	lights[0].direction = { 1.0f, -1.0f, -1.0f };
	lights[0].type = LIGHT_TYPE_SUN;
	lights[0].direction /= glm::length(lights[0].direction);

	m_globalData.proj = glm::identity<glm::mat4>();
	m_globalData.view = glm::identity<glm::mat4>();
	m_globalData.cameraPosition = glm::zero<glm::vec4>();
	mem::copy(m_globalData.lights, lights, sizeof(Light) * 16);
	pushGlobalData();

	m_instanceData.model = glm::identity<glm::mat4>();
	m_instanceData.normalMatrix = glm::identity<glm::mat4>();
	pushInstanceData();

	/*
	m_bindlessParams.setValue<Params_PBR>("pbsParams", {
		.meshTransforms = (BufferHandle)0,
		.pointLights = (BufferHandle)0,
		.camera = (BufferHandle)0
	});

	m_bindlessParams.setValue<Params_Skybox>("cameraParams", {
		.camera = (BufferHandle)0,
		.skybox = (TextureHandle)0
	});

	uint32_t maxUniformBuffers = g_vulkanBackend->physicalData.properties.limits.maxDescriptorSetUniformBuffers;
	uint32_t maxStorageBuffers = g_vulkanBackend->physicalData.properties.limits.maxDescriptorSetStorageBuffers;
	uint32_t maxSamplesImages = g_vulkanBackend->physicalData.properties.limits.maxDescriptorSetSampledImages;

	bindlessDescriptorPoolManager.setSizes({
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (float)maxUniformBuffers },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, (float)maxStorageBuffers },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, (float)maxSamplesImages }
	});

	bindlessDescriptorCache.setPoolAllocator(bindlessDescriptorPoolManager);

	m_bindlessDescriptor.bindBuffer(
		UNIFORM_BUFFER_BINDING,
		nullptr,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_ALL,
		1000,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
	);

	m_bindlessDescriptor.bindBuffer(
		STORAGE_BUFFER_BINDING,
		nullptr,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_SHADER_STAGE_ALL,
		1000,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
	);

	m_bindlessDescriptor.bindBuffer(
		COMBINED_IMAGE_BUFFER_BINDING,
		nullptr,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_ALL,
		1000,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
	);

	VkDescriptorSetLayout layout = {};
	m_bindlessDescriptor.buildLayout(layout, VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);

	std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
	std::array<VkDescriptorBindingFlags, 3> flags{};
	std::array<VkDescriptorType, 3> types{
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
	};

	for (uint32_t i = 0; i < 3; ++i) {
		bindings.at(i).binding = i;
		bindings.at(i).descriptorType = types.at(i);
		bindings.at(i).descriptorCount = 1000;
		bindings.at(i).stageFlags = VK_SHADER_STAGE_ALL;
		flags.at(i) = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
	}

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{};
	bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlags.pNext = nullptr;
	bindingFlags.pBindingFlags = flags.data();
	bindingFlags.bindingCount = 3;

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = 3;
	createInfo.pBindings = bindings.data();
	createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	createInfo.pNext = &bindingFlags;

	VkDescriptorSetLayout bindlessLayout = VK_NULL_HANDLE;
	vkCreateDescriptorSetLayout(mDevice, &createInfo, nullptr, &bindlessLay
	*/

	CommandBuffer graphicsBuffer = CommandBuffer::fromGraphics();

	generateEnvironmentMaps(graphicsBuffer);
	precomputeBRDF(graphicsBuffer);
}

// todo: this needs to be moved to a seperate class. EnvironmentMapGenerator or something idk
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

	DescriptorWriter descriptorWriter;

	SubMesh *cubeMesh = g_meshLoader->getCubeMesh();

	// =====================================================
	LLT_LOG("Generating environment map...");

	descriptorWriter.clear();

	const int ENVIRONMENT_RESOLUTION = 1024;

	m_environmentMap = g_textureManager->createCubemap(
		"environmentMap",
		ENVIRONMENT_RESOLUTION,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		4
	);

	BoundTexture hdrImage;
	hdrImage.texture = g_textureManager->getTexture("environmentHDR");
	hdrImage.sampler = g_textureManager->getSampler("linear");

	RenderInfo etcRenderInfo;
	etcRenderInfo.setSize(ENVIRONMENT_RESOLUTION, ENVIRONMENT_RESOLUTION);
	etcRenderInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, VK_NULL_HANDLE, m_environmentMap->getFormat());

	struct
	{
		glm::mat4 proj;
		glm::mat4 view;
	}
	pc;

	pc.proj = captureProjection;
	pc.view = glm::identity<glm::mat4>();

	ShaderEffect *equirectangularToCubemapShader = g_shaderManager->getEffect("equirectangularToCubemap");

	VkDescriptorSet etcDescriptorSet = m_descriptorPoolAllocator.allocate(equirectangularToCubemapShader->getDescriptorSetLayout());

	descriptorWriter.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, hdrImage.getImageInfo());
	descriptorWriter.updateSet(etcDescriptorSet);

	m_equirectangularToCubemapPipeline.bindShader(equirectangularToCubemapShader);
	m_equirectangularToCubemapPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_equirectangularToCubemapPipeline.setDepthTest(false);
	m_equirectangularToCubemapPipeline.setDepthWrite(false);
	m_equirectangularToCubemapPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_equirectangularToCubemapPipeline.buildGraphicsPipeline(etcRenderInfo);

	for (int i = 0; i < 6; i++)
	{
		pc.view = captureViews[i];

		RenderTarget target(ENVIRONMENT_RESOLUTION, ENVIRONMENT_RESOLUTION);
		target.addAttachment(m_environmentMap, i, 0);

		cmd.beginRendering(&target); // todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/

		cmd.bindPipeline(m_equirectangularToCubemapPipeline);

		cmd.bindDescriptorSets(
			0,
			1, &etcDescriptorSet,
			0, nullptr
		);

		cmd.setViewport({ 0, 0, ENVIRONMENT_RESOLUTION, ENVIRONMENT_RESOLUTION });

		cmd.pushConstants(
			VK_SHADER_STAGE_ALL_GRAPHICS,
			sizeof(pc),
			&pc
		);

		cubeMesh->render(cmd);

		cmd.endRendering();
		cmd.submit();

		vkWaitForFences(g_vulkanBackend->m_device, 1, &g_vulkanBackend->m_graphicsQueue.getCurrentFrame().inFlightFence, VK_TRUE, UINT64_MAX);
	}

	m_environmentMap->generateMipmaps();

	// =====================================================
	LLT_LOG("Generating irradiance map...");

	descriptorWriter.clear();

	const int IRRADIANCE_RESOLUTION = 32;

	m_irradianceMap = g_textureManager->createCubemap(
		"irradianceMap",
		IRRADIANCE_RESOLUTION,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		4
	);

	BoundTexture envMapImage;
	envMapImage.texture = m_environmentMap;
	envMapImage.sampler = g_textureManager->getSampler("linear");

	RenderInfo icRenderInfo;
	icRenderInfo.setSize(IRRADIANCE_RESOLUTION, IRRADIANCE_RESOLUTION);
	icRenderInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, VK_NULL_HANDLE, m_irradianceMap->getFormat());

	ShaderEffect *irradianceGenerationShader = g_shaderManager->getEffect("irradianceConvolution");

	VkDescriptorSet icDescriptorSet = m_descriptorPoolAllocator.allocate(irradianceGenerationShader->getDescriptorSetLayout());

	descriptorWriter.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, envMapImage.getImageInfo());
	descriptorWriter.updateSet(icDescriptorSet);

	m_irradianceGenerationPipeline.bindShader(irradianceGenerationShader);
	m_irradianceGenerationPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_irradianceGenerationPipeline.setDepthTest(false);
	m_irradianceGenerationPipeline.setDepthWrite(false);
	m_irradianceGenerationPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_irradianceGenerationPipeline.buildGraphicsPipeline(icRenderInfo);

	for (int i = 0; i < 6; i++)
	{
		pc.view = captureViews[i];

		RenderTarget target(IRRADIANCE_RESOLUTION, IRRADIANCE_RESOLUTION);
		target.addAttachment(m_irradianceMap, i, 0);

		cmd.beginRendering(&target); // todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/

		cmd.bindPipeline(m_irradianceGenerationPipeline);

		cmd.bindDescriptorSets(
			0,
			1, &icDescriptorSet,
			0, nullptr
		);

		cmd.setViewport({ 0, 0, IRRADIANCE_RESOLUTION, IRRADIANCE_RESOLUTION });

		cmd.pushConstants(
			VK_SHADER_STAGE_ALL_GRAPHICS,
			sizeof(pc),
			&pc
		);

		cubeMesh->render(cmd);

		cmd.endRendering();
		cmd.submit();

		vkWaitForFences(g_vulkanBackend->m_device, 1, &g_vulkanBackend->m_graphicsQueue.getCurrentFrame().inFlightFence, VK_TRUE, UINT64_MAX);
	}

	m_irradianceMap->generateMipmaps();

	// =====================================================
	LLT_LOG("Generating prefilter map...");

	descriptorWriter.clear();

	const int PREFILTER_RESOLUTION = 128;
	const int PREFILTER_MIP_LEVELS = 5;

	m_prefilterMap = g_textureManager->createCubemap(
		"prefilterMap",
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

	DynamicShaderBuffer pfParameterBuffer;
	pfParameterBuffer.init(sizeof(float) * 4 * PREFILTER_MIP_LEVELS * 6, SHADER_BUFFER_UBO);
	pfParameterBuffer.pushData(&prefilterParams, sizeof(prefilterParams));

	RenderInfo pfRenderInfo;
	pfRenderInfo.setSize(PREFILTER_RESOLUTION, PREFILTER_RESOLUTION);
	pfRenderInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, VK_NULL_HANDLE, m_prefilterMap->getFormat());

	ShaderEffect *prefilterGenerationShader = g_shaderManager->getEffect("prefilterConvolution");

	VkDescriptorSet pfDescriptorSet = m_descriptorPoolAllocator.allocate(prefilterGenerationShader->getDescriptorSetLayout());

	descriptorWriter.writeBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, pfParameterBuffer.getDescriptorInfo());
	descriptorWriter.writeImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, envMapImage.getImageInfo());
	descriptorWriter.updateSet(pfDescriptorSet);

	m_prefilterGenerationPipeline.bindShader(prefilterGenerationShader);
	m_prefilterGenerationPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_prefilterGenerationPipeline.setDepthTest(false);
	m_prefilterGenerationPipeline.setDepthWrite(false);
	m_prefilterGenerationPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_prefilterGenerationPipeline.buildGraphicsPipeline(pfRenderInfo);

	for (int mipLevel = 0; mipLevel < PREFILTER_MIP_LEVELS; mipLevel++)
	{
		int width = PREFILTER_RESOLUTION / CalcI::pow(2, mipLevel);
		int height = PREFILTER_RESOLUTION / CalcI::pow(2, mipLevel);

		float roughness = (float)mipLevel / (float)(PREFILTER_MIP_LEVELS - 1);

		prefilterParams.roughness = roughness;
		pfParameterBuffer.pushData(&prefilterParams, sizeof(prefilterParams));

		for (int i = 0; i < 6; i++)
		{
			pc.view = captureViews[i];

			RenderTarget target(width, height);
			target.addAttachment(m_prefilterMap, i, mipLevel);

			cmd.beginRendering(&target); // todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/

			cmd.bindPipeline(m_prefilterGenerationPipeline);

			uint32_t dynamicOffset = pfParameterBuffer.getDynamicOffset();

			cmd.bindDescriptorSets(
				0,
				1, &pfDescriptorSet,
				1, &dynamicOffset
			);

			cmd.setViewport({ 0, 0, (float)width, (float)height });

			cmd.pushConstants(
				VK_SHADER_STAGE_ALL_GRAPHICS,
				sizeof(pc),
				&pc
			);

			cubeMesh->render(cmd);

			cmd.endRendering();
			cmd.submit();

			vkWaitForFences(g_vulkanBackend->m_device, 1, &g_vulkanBackend->m_graphicsQueue.getCurrentFrame().inFlightFence, VK_TRUE, UINT64_MAX);
		}
	}

	m_prefilterMap->transitionLayoutSingle(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	pfParameterBuffer.cleanUp();
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
	info.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, m_brdfLUT->getStandardView(), m_brdfLUT->getFormat());

	ShaderEffect *brdfLUTShader = g_shaderManager->getEffect("brdfLUT");

	VkDescriptorSet set = m_descriptorPoolAllocator.allocate(brdfLUTShader->getDescriptorSetLayout());

	m_brdfIntegrationPipeline.bindShader(brdfLUTShader);
	m_brdfIntegrationPipeline.setVertexFormat(g_primitiveUvVertexFormat);
	m_brdfIntegrationPipeline.setDepthTest(false);
	m_brdfIntegrationPipeline.setDepthWrite(false);
	m_brdfIntegrationPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_brdfIntegrationPipeline.buildGraphicsPipeline(info);

	cmd.beginRendering(info);

	cmd.bindPipeline(m_brdfIntegrationPipeline);

	cmd.bindDescriptorSets(
		0,
		1, &set,
		0, nullptr
	);

	cmd.setViewport({ 0, 0, BRDF_RESOLUTION, BRDF_RESOLUTION });

	quadMesh->render(cmd);

	cmd.endRendering();
	cmd.submit();

	vkWaitForFences(g_vulkanBackend->m_device, 1, &g_vulkanBackend->m_graphicsQueue.getCurrentFrame().inFlightFence, VK_TRUE, UINT64_MAX);
}

// todo: way to specifically decide what global buffer gets applied to what bindings in the technique

Material *MaterialSystem::buildMaterial(MaterialData &data)
{
	if (m_materials.contains(data.getHash())) {
		return m_materials.get(data.getHash());
	}

	Technique &technique = m_techniques.get(data.technique);

	Material *material = new Material();
	material->m_parameterBuffer = g_shaderBufferManager->createUBO();

	if (data.parameterSize > 0)
	{
		material->m_parameterBuffer->pushData(data.parameters, data.parameterSize);
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

	material->m_vertexFormat = technique.vertexFormat;
	material->m_textures = data.textures;

	DescriptorWriter descriptorWriter;
	descriptorWriter.writeBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,			 m_globalBuffer		->getDescriptorInfo());
	descriptorWriter.writeBuffer(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,			 m_instanceBuffer	->getDescriptorInfo());
	descriptorWriter.writeBuffer(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, material->m_parameterBuffer	->getDescriptorInfo());

	// write the irradiance map
	BoundTexture irradianceMap = {};
	irradianceMap.texture = g_textureManager->getTexture("irradianceMap");
	irradianceMap.sampler = g_textureManager->getSampler("linear");

	descriptorWriter.writeImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, irradianceMap.getImageInfo());

	// write the prefilter map
	BoundTexture prefilterMap = {};
	prefilterMap.texture = g_textureManager->getTexture("prefilterMap");
	prefilterMap.sampler = g_textureManager->getSampler("linear");

	descriptorWriter.writeImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, prefilterMap.getImageInfo());

	// write the brdf LUT
	BoundTexture brdfTexture = {};
	brdfTexture.texture = g_textureManager->getTexture("brdfIntegration");
	brdfTexture.sampler = g_textureManager->getSampler("linear");

	descriptorWriter.writeImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, brdfTexture.getImageInfo());

	// write the rest of the textures
	for (int i = 0; i < data.textures.size(); i++)
	{
		descriptorWriter.writeImage(6 + i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, data.textures[i].getImageInfo());
	}

	for (int i = 0; i < SHADER_PASS_MAX_ENUM; i++)
	{
		if (!technique.passes[i])
			continue;

		material->m_passes[i].shader = technique.passes[i];

		material->m_passes[i].set = m_descriptorPoolAllocator.allocate(technique.passes[i]->getDescriptorSetLayout());
		descriptorWriter.updateSet(material->m_passes[i].set);

		material->m_passes[i].pipeline.bindShader(technique.passes[i]);
		material->m_passes[i].pipeline.setVertexFormat(technique.vertexFormat);
		material->m_passes[i].pipeline.setDepthTest(technique.depthTest);
		material->m_passes[i].pipeline.setDepthWrite(technique.depthWrite);
		material->m_passes[i].pipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	}

	m_materials.insert(data.getHash(), material);

	return material;
}

void MaterialSystem::loadDefaultTechniques()
{
	Technique pbrTechnique;
	pbrTechnique.passes[SHADER_PASS_FORWARD] = g_shaderManager->getEffect("texturedPBR_opaque");
	pbrTechnique.passes[SHADER_PASS_SHADOW] = nullptr;
	pbrTechnique.vertexFormat = g_modelVertexFormat;
	addTechnique("texturedPBR_opaque", pbrTechnique);
}

void MaterialSystem::addTechnique(const String &name, const Technique &technique)
{
	m_techniques.insert(name, technique);
}

void MaterialSystem::pushGlobalData()
{
	m_globalBuffer->pushData(&m_globalData, sizeof(m_globalData));
}

void MaterialSystem::pushInstanceData()
{
	m_instanceBuffer->pushData(&m_instanceData, sizeof(m_instanceData));
}

DynamicShaderBuffer *MaterialSystem::getGlobalBuffer() const
{
	return m_globalBuffer;
}

DynamicShaderBuffer *MaterialSystem::getInstanceBuffer() const
{
	return m_instanceBuffer;
}
