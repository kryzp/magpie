#include "material_system.h"
#include "shader_mgr.h"
#include "vertex_descriptor.h"
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
	, globalParameters()
{
}

MaterialSystem::~MaterialSystem()
{
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
	m_descriptorPoolAllocator.setSizes(64 * mgc::FRAMES_IN_FLIGHT, {
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

	lights[0].position = { 0.0f, 0.0f, 0.0f };
	lights[0].radius = 0.0f;
	lights[0].attenuation = 0.0f;
	lights[0].colour = { 1.0f, 1.0f, 1.0f };
	lights[0].direction = { 1.0f, -1.0f, -1.0f };
	lights[0].type = LIGHT_TYPE_SUN;

	lights[0].direction /= glm::length(lights[0].direction);

	globalParameters.setValue<glm::mat4>("projMatrix", glm::identity<glm::mat4>());
	globalParameters.setValue<glm::mat4>("viewMatrix", glm::identity<glm::mat4>());
	globalParameters.setValue<glm::vec4>("viewPos", glm::zero<glm::vec4>());
	globalParameters.setArray<Light>("lights", lights, 16);
	updateGlobalBuffer();

	instanceParameters.setValue<glm::mat4>("modelMatrix", glm::identity<glm::mat4>());
	instanceParameters.setValue<glm::mat4>("normalMatrix", glm::identity<glm::mat4>());
	updateInstanceBuffer();

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
}

void MaterialSystem::updateGlobalBuffer()
{
	m_globalBuffer->pushData(globalParameters);
}

void MaterialSystem::updateInstanceBuffer()
{
	m_instanceBuffer->pushData(instanceParameters);
}

const ShaderBuffer* MaterialSystem::getGlobalBuffer() const
{
	return m_globalBuffer;
}

const ShaderBuffer* MaterialSystem::getInstanceBuffer() const
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
	addTechnique("skybox", skyboxTechnique);
}

// todo: way to specifically decide what global buffer gets applied to what bindings in the technique

Material* MaterialSystem::buildMaterial(MaterialData& data)
{
	data.parameters.setValue<float>("temp", 0.0f);

	if (m_materials.contains(data.getHash())) {
		return m_materials.get(data.getHash());
	}

	Technique& technique = m_techniques.get(data.technique);

	Material* material = new Material();

	material->parameterBuffer = g_shaderBufferManager->createUBO();
	material->parameterBuffer->pushData(data.parameters);

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

	for (int i = 0; i < data.textures.size(); i++)
	{
		descriptorLayoutBuilder.bind(i + 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorWriter.writeImage(i + 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, data.textures[i].getImageInfo());
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

		material->passes[i].pipeline.setVertexDescriptor(technique.vertexFormat);
		material->passes[i].pipeline.setDepthTest(data.depthTest);
		material->passes[i].pipeline.setDepthWrite(data.depthWrite);
		material->passes[i].pipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
		material->passes[i].pipeline.setDescriptorSetLayout(i == SHADER_PASS_FORWARD ? descriptorLayout : VK_NULL_HANDLE);

		material->passes[i].effect = technique.passes[i];

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
