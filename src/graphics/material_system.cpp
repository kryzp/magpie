#include "material_system.h"
#include "shader_mgr.h"
#include "vertex_format.h"
#include "light.h"
#include "backend.h"

#include "../../res/shaders/raster/common_fxc.inc"

#include "../math/calc.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

llt::MaterialSystem* llt::g_materialSystem = nullptr;

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
	, m_environmentMap()
	, m_irradianceMap()
	, m_prefilterMap()
	, m_brdfIntegration()
	, m_equirectangularToCubemapPipeline()
	, m_irradianceGenerationPipeline()
	, m_prefilterGenerationPipeline()
	, m_brdfIntegrationPipeline()
	, m_quadMesh()
	, m_cubeMesh()
{
}

MaterialSystem::~MaterialSystem()
{
	delete m_quadMesh;
	delete m_cubeMesh;

	for (auto& [id, mat] : m_materials) {
		delete mat;
	}

	m_materials.clear();
	m_techniques.clear();

	m_descriptorCache.cleanUp();
	m_descriptorPoolAllocator.cleanUp();
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

//	lights[0].position = { 0.0f, 5.0f, 0.0f };
//	lights[0].radius = 0.0f;
//	lights[0].attenuation = 0.0f;
//	lights[0].colour = { 1.0f, 1.0f, 1.0f };
//	lights[0].direction = { 1.0f, -1.0f, -1.0f };
//	lights[0].type = LIGHT_TYPE_SUN;
//	lights[0].direction /= glm::length(lights[0].direction);

	m_globalBuffer->getParameters().setValue<glm::mat4>("projMatrix", glm::identity<glm::mat4>());
	m_globalBuffer->getParameters().setValue<glm::mat4>("viewMatrix", glm::identity<glm::mat4>());
	m_globalBuffer->getParameters().setValue<glm::vec4>("viewPos", glm::zero<glm::vec4>());
	m_globalBuffer->getParameters().setArray<Light>("lights", lights, 16);
	m_globalBuffer->pushParameters();

	m_instanceBuffer->getParameters().setValue<glm::mat4>("modelMatrix", glm::identity<glm::mat4>());
	m_instanceBuffer->getParameters().setValue<glm::mat4>("normalMatrix", glm::identity<glm::mat4>());
	m_instanceBuffer->pushParameters();

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

	createQuadMesh();
	createCubeMesh();

	CommandBuffer graphicsBuffer = CommandBuffer::fromGraphics();

	generateEnvironmentMaps(graphicsBuffer);
	precomputeBRDF(graphicsBuffer);
}

void MaterialSystem::createQuadMesh()
{
	Vector<PrimitiveUVVertex> vertices =
	{
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } },
		{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
		{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f } },
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f } }
	};

	Vector<uint16_t> indices =
	{
		0, 1, 2,
		0, 2, 3
	};

	m_quadMesh = new SubMesh();
	m_quadMesh->build(
		sizeof(PrimitiveUVVertex),
		vertices.data(), vertices.size(),
		indices.data(), indices.size()
	);
}

void MaterialSystem::createCubeMesh()
{
	Vector<PrimitiveVertex> vertices =
	{
		{ { -1.0f,  1.0f, -1.0f } },
		{ { -1.0f, -1.0f, -1.0f } },
		{ {  1.0f, -1.0f, -1.0f } },
		{ {  1.0f,  1.0f, -1.0f } },
		{ { -1.0f,  1.0f,  1.0f } },
		{ { -1.0f, -1.0f,  1.0f } },
		{ {  1.0f, -1.0f,  1.0f } },
		{ {  1.0f,  1.0f,  1.0f } }
	};

	Vector<uint16_t> indices =
	{
		0, 1, 2,
		2, 3, 0,

		4, 6, 5,
		7, 6, 4,

		4, 5, 1,
		1, 0, 4,

		3, 2, 6,
		6, 7, 3,

		4, 0, 3,
		3, 7, 4,

		1, 5, 6,
		6, 2, 1
	};

	m_cubeMesh = new SubMesh();
	m_cubeMesh->build(
		sizeof(PrimitiveVertex),
		vertices.data(), vertices.size(),
		indices.data(), indices.size()
	);
}

// todo: this needs to be moved to a seperate class. EnvironmentMapGenerator or something idk
// -> also lots of repeating code.
void MaterialSystem::generateEnvironmentMaps(CommandBuffer& buffer)
{
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

	glm::mat4 captureViews[] = {
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec3(0.0f,-1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f,-1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,-1.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 0.0f, 1.0f), glm::vec3(0.0f,-1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 0.0f,-1.0f), glm::vec3(0.0f,-1.0f, 0.0f))
	};

	// todo: shader loading needs to be moved into one function for ease of use and then just use ->get(...) everywhere else
	ShaderProgram* primitiveShader = g_shaderManager->create("primitiveVS", "../../res/shaders/raster/primitive_vs.spv", VK_SHADER_STAGE_VERTEX_BIT);
	ShaderProgram* equirectangularToCubemapShader = g_shaderManager->create("equirectangularToCubemap", "../../res/shaders/raster/equirectangular_to_cubemap.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	ShaderProgram* irradianceConvolutionShader = g_shaderManager->create("irradianceConvolution", "../../res/shaders/raster/irradiance_convolution.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	ShaderProgram* prefilterConvolutionShader = g_shaderManager->create("prefilterConvolution", "../../res/shaders/raster/prefilter_convolution.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	DescriptorLayoutBuilder descriptorLayoutBuilder;
	DescriptorWriter descriptorWriter;

	// =====================================================
	LLT_LOG("Generating environment map...");

	const int ENVIRONMENT_RESOLUTION = 1024;

	m_environmentMap = g_textureManager->createCubemap("environmentMap", ENVIRONMENT_RESOLUTION, VK_FORMAT_R32G32B32A32_SFLOAT, 4);

	BoundTexture hdrImage;
	hdrImage.texture = g_textureManager->getTexture("environmentHDR");
	hdrImage.sampler = g_textureManager->getSampler("linear");

	descriptorLayoutBuilder.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	descriptorWriter.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, hdrImage.getImageInfo());

	VkDescriptorSetLayout etcDescriptorLayout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, &m_descriptorCache);
	VkDescriptorSet etcDescriptorSet = m_descriptorPoolAllocator.allocate(etcDescriptorLayout);

	descriptorWriter.updateSet(etcDescriptorSet);

	RenderInfo etcRenderInfo;
	etcRenderInfo.setSize(ENVIRONMENT_RESOLUTION, ENVIRONMENT_RESOLUTION);
	etcRenderInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, VK_NULL_HANDLE, m_environmentMap->getFormat(), VK_NULL_HANDLE);

	ShaderParameters pc;
	pc.setValue<glm::mat4>("proj", captureProjection);
	pc.setValue<glm::mat4>("view", glm::identity<glm::mat4>());

	g_vulkanBackend->setPushConstants(pc);

	m_equirectangularToCubemapPipeline.bindShader(primitiveShader);
	m_equirectangularToCubemapPipeline.bindShader(equirectangularToCubemapShader);
	m_equirectangularToCubemapPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_equirectangularToCubemapPipeline.setDepthTest(false);
	m_equirectangularToCubemapPipeline.setDepthWrite(false);
	m_equirectangularToCubemapPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_equirectangularToCubemapPipeline.setDescriptorSetLayout(etcDescriptorLayout);
	m_equirectangularToCubemapPipeline.setViewport({ 0, 0, ENVIRONMENT_RESOLUTION, ENVIRONMENT_RESOLUTION });
	m_equirectangularToCubemapPipeline.create(&etcRenderInfo);

	for (int i = 0; i < 6; i++)
	{
		pc.setValue<glm::mat4>("view", captureViews[i]);
		g_vulkanBackend->setPushConstants(pc);

		RenderTarget target(ENVIRONMENT_RESOLUTION, ENVIRONMENT_RESOLUTION);
		target.addAttachment(m_environmentMap, i, 0);

		buffer.beginRendering(&target); // todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/

		m_equirectangularToCubemapPipeline.bind(buffer);

		vkCmdBindDescriptorSets(
			buffer.getBuffer(),
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_equirectangularToCubemapPipeline.getPipelineLayout(),
			0,
			1, &etcDescriptorSet,
			0, nullptr
		);

		m_equirectangularToCubemapPipeline.render(buffer, *m_cubeMesh);

		buffer.endRendering();
		buffer.submit();
	}

	m_environmentMap->generateMipmaps();

	// =====================================================
	LLT_LOG("Generating irradiance map...");

	const int IRRADIANCE_RESOLUTION = 32;

	descriptorLayoutBuilder.clear();
	descriptorWriter.clear();

	m_irradianceMap = g_textureManager->createCubemap("irradianceMap", IRRADIANCE_RESOLUTION, VK_FORMAT_R32G32B32A32_SFLOAT, 4);

	BoundTexture envMapImage;
	envMapImage.texture = m_environmentMap;
	envMapImage.sampler = g_textureManager->getSampler("linear");

	descriptorLayoutBuilder.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	descriptorWriter.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, envMapImage.getImageInfo());

	VkDescriptorSetLayout icDescriptorLayout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, &m_descriptorCache);
	VkDescriptorSet icDescriptorSet = m_descriptorPoolAllocator.allocate(icDescriptorLayout);

	descriptorWriter.updateSet(icDescriptorSet);

	RenderInfo icRenderInfo;
	icRenderInfo.setSize(IRRADIANCE_RESOLUTION, IRRADIANCE_RESOLUTION);
	icRenderInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, VK_NULL_HANDLE, m_irradianceMap->getFormat(), VK_NULL_HANDLE);

	m_irradianceGenerationPipeline.bindShader(primitiveShader);
	m_irradianceGenerationPipeline.bindShader(irradianceConvolutionShader);
	m_irradianceGenerationPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_irradianceGenerationPipeline.setDepthTest(false);
	m_irradianceGenerationPipeline.setDepthWrite(false);
	m_irradianceGenerationPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_irradianceGenerationPipeline.setDescriptorSetLayout(icDescriptorLayout);
	m_irradianceGenerationPipeline.setViewport({ 0, 0, IRRADIANCE_RESOLUTION, IRRADIANCE_RESOLUTION });
	m_irradianceGenerationPipeline.create(&icRenderInfo);

	for (int i = 0; i < 6; i++)
	{
		pc.setValue<glm::mat4>("view", captureViews[i]);
		g_vulkanBackend->setPushConstants(pc);

		RenderTarget target(IRRADIANCE_RESOLUTION, IRRADIANCE_RESOLUTION);
		target.addAttachment(m_irradianceMap, i, 0);

		buffer.beginRendering(&target); // todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/

		m_irradianceGenerationPipeline.bind(buffer);

		vkCmdBindDescriptorSets(
			buffer.getBuffer(),
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_irradianceGenerationPipeline.getPipelineLayout(),
			0,
			1, &icDescriptorSet,
			0, nullptr
		);

		m_irradianceGenerationPipeline.render(buffer, *m_cubeMesh);

		buffer.endRendering();
		buffer.submit();
	}

	m_irradianceMap->generateMipmaps();

	// =====================================================
	LLT_LOG("Generating prefilter map...");

	const int PREFILTER_RESOLUTION = 128;
	const int PREFILTER_MIP_LEVELS = 5;
	
	descriptorLayoutBuilder.clear();
	descriptorWriter.clear();

	m_prefilterMap = g_textureManager->createCubemap("prefilterMap", PREFILTER_RESOLUTION, VK_FORMAT_R32G32B32A32_SFLOAT, PREFILTER_MIP_LEVELS);

	DynamicShaderBuffer pfParameterBuffer;
	pfParameterBuffer.init(sizeof(float) * 4 * PREFILTER_MIP_LEVELS * 6, SHADER_BUFFER_UBO);
	pfParameterBuffer.getParameters().setValue<float>("roughness", 0.0f);
	pfParameterBuffer.getParameters().setValue<float>("_padding0", 0.0f);
	pfParameterBuffer.getParameters().setValue<float>("_padding1", 0.0f);
	pfParameterBuffer.getParameters().setValue<float>("_padding2", 0.0f);
	pfParameterBuffer.pushParameters();

	descriptorLayoutBuilder.bind(0, pfParameterBuffer.getDescriptorType());
	descriptorWriter.writeBuffer(0, pfParameterBuffer.getDescriptorType(), pfParameterBuffer.getDescriptorInfo());

	descriptorLayoutBuilder.bind(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	descriptorWriter.writeImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, envMapImage.getImageInfo());

	VkDescriptorSetLayout pfDescriptorLayout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, &m_descriptorCache);
	VkDescriptorSet pfDescriptorSet = m_descriptorPoolAllocator.allocate(pfDescriptorLayout);

	descriptorWriter.updateSet(pfDescriptorSet);

	RenderInfo pfRenderInfo;
	pfRenderInfo.setSize(PREFILTER_RESOLUTION, PREFILTER_RESOLUTION);
	pfRenderInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, VK_NULL_HANDLE, m_prefilterMap->getFormat(), VK_NULL_HANDLE);

	m_prefilterGenerationPipeline.bindShader(primitiveShader);
	m_prefilterGenerationPipeline.bindShader(prefilterConvolutionShader);
	m_prefilterGenerationPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_prefilterGenerationPipeline.setDepthTest(false);
	m_prefilterGenerationPipeline.setDepthWrite(false);
	m_prefilterGenerationPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_prefilterGenerationPipeline.setDescriptorSetLayout(pfDescriptorLayout);
	m_prefilterGenerationPipeline.setViewport({ 0, 0, PREFILTER_RESOLUTION, PREFILTER_RESOLUTION });
	m_prefilterGenerationPipeline.create(&pfRenderInfo);

	for (int mipLevel = 0; mipLevel < PREFILTER_MIP_LEVELS; mipLevel++)
	{
		int width = PREFILTER_RESOLUTION / CalcI::pow(2, mipLevel);
		int height = PREFILTER_RESOLUTION / CalcI::pow(2, mipLevel);

		m_prefilterGenerationPipeline.setViewport({ 0, 0, (float)width, (float)height });

		float roughness = (float)mipLevel / (float)(PREFILTER_MIP_LEVELS - 1);

		pfParameterBuffer.getParameters().setValue<float>("roughness", roughness);
		pfParameterBuffer.pushParameters();

		for (int i = 0; i < 6; i++)
		{
			pc.setValue<glm::mat4>("view", captureViews[i]);
			g_vulkanBackend->setPushConstants(pc);

			RenderTarget target(PREFILTER_RESOLUTION, PREFILTER_RESOLUTION);
			target.addAttachment(m_prefilterMap, i, mipLevel);

			buffer.beginRendering(&target); // todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/

			m_prefilterGenerationPipeline.bind(buffer);

			uint32_t dynamicOffsets[] = {
				pfParameterBuffer.getDynamicOffset()
			};

			vkCmdBindDescriptorSets(
				buffer.getBuffer(),
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_prefilterGenerationPipeline.getPipelineLayout(),
				0,
				1, &pfDescriptorSet,
				LLT_ARRAY_LENGTH(dynamicOffsets), dynamicOffsets
			);

			m_prefilterGenerationPipeline.render(buffer, *m_cubeMesh);

			buffer.endRendering();
			buffer.submit();
		}
	}

	m_prefilterMap->transitionLayoutSingle(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	pfParameterBuffer.cleanUp();

	g_vulkanBackend->resetPushConstants();
}

void MaterialSystem::precomputeBRDF(CommandBuffer& buffer)
{
	LLT_LOG("Precomputing BRDF...");

	const int BRDF_RESOLUTION = 512;

	ShaderProgram* primitiveQuadShader = g_shaderManager->create("primitiveQuadVS", "../../res/shaders/raster/primitive_quad_vs.spv", VK_SHADER_STAGE_VERTEX_BIT);
	ShaderProgram* brdfIntegratorShader = g_shaderManager->create("brdfIntegrator", "../../res/shaders/raster/brdf_integrator.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	m_brdfIntegration = g_textureManager->createAttachment("brdfIntegration", BRDF_RESOLUTION, BRDF_RESOLUTION, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_TILING_LINEAR);

	DescriptorLayoutBuilder descriptorLayoutBuilder;

	VkDescriptorSetLayout layout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, &m_descriptorCache);
	VkDescriptorSet set = m_descriptorPoolAllocator.allocate(layout);

	RenderInfo info;
	info.setSize(BRDF_RESOLUTION, BRDF_RESOLUTION);
	info.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, m_brdfIntegration->getStandardView(), m_brdfIntegration->getFormat(), VK_NULL_HANDLE);

	m_brdfIntegrationPipeline.bindShader(primitiveQuadShader);
	m_brdfIntegrationPipeline.bindShader(brdfIntegratorShader);
	m_brdfIntegrationPipeline.setVertexFormat(g_primitiveUvVertexFormat);
	m_brdfIntegrationPipeline.setDepthTest(false);
	m_brdfIntegrationPipeline.setDepthWrite(false);
	m_brdfIntegrationPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_brdfIntegrationPipeline.setDescriptorSetLayout(layout);
	m_brdfIntegrationPipeline.setViewport({ 0, 0, BRDF_RESOLUTION, BRDF_RESOLUTION });
	m_brdfIntegrationPipeline.create(&info);

	buffer.beginRendering(info);

	m_brdfIntegrationPipeline.bind(buffer);

	vkCmdBindDescriptorSets(
		buffer.getBuffer(),
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_brdfIntegrationPipeline.getPipelineLayout(),
		0,
		1, &set,
		0, nullptr
	);

	m_brdfIntegrationPipeline.render(buffer, *m_quadMesh);

	buffer.endRendering();
	buffer.submit();
}

DynamicShaderBuffer* MaterialSystem::getGlobalBuffer() const
{
	return m_globalBuffer;
}

DynamicShaderBuffer* MaterialSystem::getInstanceBuffer() const
{
	return m_instanceBuffer;
}

void MaterialSystem::loadDefaultTechniques()
{
	ShaderProgram* genericVertex	= g_shaderManager->create("genericVertex",	"../../res/shaders/raster/model_vs.spv",		VK_SHADER_STAGE_VERTEX_BIT);
	ShaderProgram* pbrFragment		= g_shaderManager->create("pbrFragment",	"../../res/shaders/raster/texturedPBR.spv",		VK_SHADER_STAGE_FRAGMENT_BIT);
	ShaderProgram* skyboxFragment	= g_shaderManager->create("skyboxFragment",	"../../res/shaders/raster/skybox.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);

	ShaderEffect* pbrEffect = g_shaderManager->createEffect();
	pbrEffect->stages.pushBack(genericVertex);
	pbrEffect->stages.pushBack(pbrFragment);

	ShaderEffect* skyboxEffect = g_shaderManager->createEffect();
	skyboxEffect->stages.pushBack(genericVertex);
	skyboxEffect->stages.pushBack(skyboxFragment);

	Technique pbrTechnique;
	pbrTechnique.passes[SHADER_PASS_FORWARD] = pbrEffect;
	pbrTechnique.passes[SHADER_PASS_SHADOW] = nullptr;
	pbrTechnique.vertexFormat = g_modelVertexFormat;
	addTechnique("texturedPBR_opaque", pbrTechnique);

	Technique skyboxTechnique;
	skyboxTechnique.passes[SHADER_PASS_FORWARD] = skyboxEffect;
	skyboxTechnique.passes[SHADER_PASS_SHADOW] = nullptr;
	skyboxTechnique.vertexFormat = g_modelVertexFormat;
	skyboxTechnique.depthTest = false;
	skyboxTechnique.depthWrite = false;
	addTechnique("skybox", skyboxTechnique);
}

// todo: way to specifically decide what global buffer gets applied to what bindings in the technique

Material* MaterialSystem::buildMaterial(MaterialData& data)
{
	if (m_materials.contains(data.getHash())) {
		return m_materials.get(data.getHash());
	}

	Technique& technique = m_techniques.get(data.technique);

	Material* material = new Material();

	data.parameters.setValue<float>("asdf", 0.0f);

	material->parameterBuffer = g_shaderBufferManager->createUBO();
	material->parameterBuffer->setParameters(data.parameters);
	material->parameterBuffer->pushParameters();

	material->vertexFormat = technique.vertexFormat;
	material->textures = data.textures;

	DescriptorLayoutBuilder descriptorLayoutBuilder;
	descriptorLayoutBuilder.bind(0, m_globalBuffer->getDescriptorType());
	descriptorLayoutBuilder.bind(1, m_instanceBuffer->getDescriptorType());
	descriptorLayoutBuilder.bind(2, material->parameterBuffer->getDescriptorType());

	DescriptorWriter descriptorWriter;
	descriptorWriter.writeBuffer(0, m_globalBuffer->getDescriptorType(), m_globalBuffer->getDescriptorInfo());
	descriptorWriter.writeBuffer(1, m_instanceBuffer->getDescriptorType(), m_instanceBuffer->getDescriptorInfo());
	descriptorWriter.writeBuffer(2, material->parameterBuffer->getDescriptorType(), material->parameterBuffer->getDescriptorInfo());

	// bind irradiance map
	BoundTexture irradianceMap = {};
	irradianceMap.texture = g_textureManager->getTexture("irradianceMap");
	irradianceMap.sampler = g_textureManager->getSampler("linear");

	descriptorLayoutBuilder.bind(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	descriptorWriter.writeImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, irradianceMap.getImageInfo());

	// bind prefilter map
	BoundTexture prefilterMap = {};
	prefilterMap.texture = g_textureManager->getTexture("prefilterMap");
	prefilterMap.sampler = g_textureManager->getSampler("linear");

	descriptorLayoutBuilder.bind(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	descriptorWriter.writeImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, prefilterMap.getImageInfo());

	// bind brdf LUT
	BoundTexture brdfTexture = {};
	brdfTexture.texture = g_textureManager->getTexture("brdfIntegration");
	brdfTexture.sampler = g_textureManager->getSampler("linear");

	descriptorLayoutBuilder.bind(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	descriptorWriter.writeImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, brdfTexture.getImageInfo());

	// rest of the textures
	for (int i = 0; i < data.textures.size(); i++)
	{
		descriptorLayoutBuilder.bind(6 + i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorWriter.writeImage(6 + i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, data.textures[i].getImageInfo());
	}

	VkDescriptorSetLayout descriptorLayout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, &m_descriptorCache);
	
	for (int i = 0; i < SHADER_PASS_MAX_ENUM; i++)
	{
		if (!technique.passes[i]) {
			continue;
		}

		for (int j = 0; j < technique.passes[i]->stages.size(); j++) {
			material->passes[i].pipeline.bindShader(technique.passes[i]->stages[j]);
		}

		material->passes[i].pipeline.setVertexFormat(technique.vertexFormat);
		material->passes[i].pipeline.setDepthTest(technique.depthTest);
		material->passes[i].pipeline.setDepthWrite(technique.depthWrite);
		material->passes[i].pipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
		material->passes[i].pipeline.setDescriptorSetLayout(i == SHADER_PASS_FORWARD ? descriptorLayout : VK_NULL_HANDLE);
		material->passes[i].pipeline.setMSAA(g_vulkanBackend->m_maxMsaaSamples); // todo temp

		material->passes[i].shader = technique.passes[i];

		material->passes[i].set = m_descriptorPoolAllocator.allocate(descriptorLayout);

		descriptorWriter.updateSet(material->passes[i].set);
	}

	m_materials.insert(data.getHash(), material);
	return material;
}

void MaterialSystem::addTechnique(const String& name, const Technique& technique)
{
	m_techniques.insert(name, technique);
}
