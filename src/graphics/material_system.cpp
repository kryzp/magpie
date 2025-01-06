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

	bindlessDescriptorPoolManager.cleanUp();
	bindlessDescriptorCache.cleanUp();
}

void MaterialSystem::initBuffers()
{
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

	bindlessDescriptorCache.setPoolManager(bindlessDescriptorPoolManager);

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

	/*
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
	ShaderProgram* genericVertex	= g_shaderManager->create("genericVertex",	"../res/shaders/raster/model_vs.spv",			VK_SHADER_STAGE_VERTEX_BIT);
	ShaderProgram* pbrFragment		= g_shaderManager->create("pbrFragment",	"../res/shaders/raster/texturedPBR.spv",		VK_SHADER_STAGE_FRAGMENT_BIT);
	ShaderProgram* skyboxFragment	= g_shaderManager->create("skyboxFragment",	"../res/shaders/raster/skybox.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);

	ShaderEffect* pbrEffect = g_shaderManager->createEffect();
	pbrEffect->stages.pushBack(genericVertex);
	pbrEffect->stages.pushBack(pbrFragment);

	ShaderEffect* skyboxEffect = g_shaderManager->createEffect();
	skyboxEffect->stages.pushBack(genericVertex);
	skyboxEffect->stages.pushBack(skyboxFragment);

	Technique pbrTechnique;
	Technique skyboxTechnique;

	pbrTechnique.setPass(SHADER_PASS_FORWARD, pbrEffect);
	pbrTechnique.setPass(SHADER_PASS_SHADOW, nullptr);
	pbrTechnique.setVertexFormat(g_modelVertexFormat);

	skyboxTechnique.setPass(SHADER_PASS_FORWARD, skyboxEffect);
	skyboxTechnique.setPass(SHADER_PASS_SHADOW, nullptr);
	skyboxTechnique.setVertexFormat(g_modelVertexFormat);

	addTechnique("texturedPBR_opaque", pbrTechnique);
	addTechnique("skybox", skyboxTechnique);
}

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

	material->textures = data.textures;

	material->pipeline.setVertexDescriptor(technique.getVertexFormat());
	material->pipeline.setDepthTest(data.depthTest);
	material->pipeline.setDepthWrite(data.depthWrite);
	material->pipeline.bindShader(technique.getPass(SHADER_PASS_FORWARD)->stages[0]);
	material->pipeline.bindShader(technique.getPass(SHADER_PASS_FORWARD)->stages[1]);
	material->pipeline.setCullMode(VK_CULL_MODE_BACK_BIT);

	material->pipeline.bindBuffer(0, m_globalBuffer); // todo: way to specifically decide what global buffer gets applied to what bindings in the technique
	material->pipeline.bindBuffer(1, m_instanceBuffer);
	material->pipeline.bindBuffer(2, material->parameterBuffer);

	for (int i = 0; i < data.textures.size(); i++)
	{
		material->pipeline.bindTexture(
			i + 3,
			data.textures[i].texture,
			data.textures[i].sampler
		);
	}

	m_materials.insert(data.getHash(), material);
	return material;
}

void MaterialSystem::addTechnique(const String& name, const Technique& technique)
{
	m_techniques.insert(name, technique);
}
