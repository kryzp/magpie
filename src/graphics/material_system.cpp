#include "material_system.h"
#include "shader_mgr.h"
#include "vertex_descriptor.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

llt::MaterialSystem* llt::g_materialSystem = nullptr;

using namespace llt;

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
}

void MaterialSystem::initBuffers()
{
	m_globalBuffer = g_shaderBufferManager->createUBO();
	m_instanceBuffer = g_shaderBufferManager->createUBO();

	globalParameters.setMat4("projMatrix", glm::identity<glm::mat4>());
	globalParameters.setMat4("currViewMatrix", glm::identity<glm::mat4>());
	globalParameters.setMat4("prevViewMatrix", glm::identity<glm::mat4>());
	globalParameters.setVec3("viewPos", glm::zero<glm::vec3>());

	instanceParameters.setMat4("currModelMatrix", glm::identity<glm::mat4>());
	instanceParameters.setMat4("prevModelMatrix", glm::identity<glm::mat4>());

	updateGlobalBuffer();
	updateInstanceBuffer();
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
	ShaderProgram* genericVertex	= g_shaderManager->create("genericVertex",	"../res/shaders/raster/generic_vertex.spv",			VK_SHADER_STAGE_VERTEX_BIT);
	ShaderProgram* pbrFragment		= g_shaderManager->create("pbrFragment",	"../res/shaders/raster/texturedPBR_fragment.spv",	VK_SHADER_STAGE_FRAGMENT_BIT);
	ShaderProgram* skyboxFragment	= g_shaderManager->create("skyboxFragment",	"../res/shaders/raster/fragment_skybox.spv",		VK_SHADER_STAGE_FRAGMENT_BIT);

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
	pbrTechnique.setVertexFormat(g_modelVertex);

	skyboxTechnique.setPass(SHADER_PASS_FORWARD, skyboxEffect);
	skyboxTechnique.setPass(SHADER_PASS_SHADOW, nullptr);
	skyboxTechnique.setVertexFormat(g_modelVertex);

	addTechnique("texturedPBR_opaque", pbrTechnique);
	addTechnique("skybox", skyboxTechnique);
}

Material* MaterialSystem::buildMaterial(MaterialData& data)
{
	data.parameters.setFloat("temp", 0.0f);

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
