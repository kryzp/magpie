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
{
}

ShaderMgr::~ShaderMgr()
{
	for (auto &effect : m_effects) {
		delete effect;
	}

	m_effects.clear();

	for (auto& [name, shader] : m_shaderCache) {
		delete shader;
	}

	m_shaderCache.clear();
}

void ShaderMgr::loadDefaultShaders()
{
	load("primitiveVS",					"../res/shaders/raster/primitive_vs.spv",				VK_SHADER_STAGE_VERTEX_BIT);
	load("primitiveQuadVS",				"../res/shaders/raster/primitive_quad_vs.spv",			VK_SHADER_STAGE_VERTEX_BIT);
	load("genericVertex",				"../res/shaders/raster/model_vs.spv",					VK_SHADER_STAGE_VERTEX_BIT);

	load("equirectangularToCubemap",	"../res/shaders/raster/equirectangular_to_cubemap.spv",	VK_SHADER_STAGE_FRAGMENT_BIT);
	load("irradianceConvolution",		"../res/shaders/raster/irradiance_convolution.spv",		VK_SHADER_STAGE_FRAGMENT_BIT);
	load("prefilterConvolution",		"../res/shaders/raster/prefilter_convolution.spv",		VK_SHADER_STAGE_FRAGMENT_BIT);
	load("brdfIntegrator",				"../res/shaders/raster/brdf_integrator.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
	load("pbrFragment",					"../res/shaders/raster/texturedPBR.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
	load("skyboxFragment",				"../res/shaders/raster/skybox.spv",						VK_SHADER_STAGE_FRAGMENT_BIT);
}

ShaderProgram *ShaderMgr::get(const String &name)
{
	if (m_shaderCache.contains(name)) {
		return m_shaderCache.get(name);
	}

	return nullptr;
}

ShaderProgram *ShaderMgr::load(const String &name, const String &source, VkShaderStageFlagBits type)
{
	if (m_shaderCache.contains(name)) {
		return m_shaderCache.get(name);
	}

	FileStream fs(source.cstr(), "r");
	Vector<char> sourceData(fs.size());
	fs.read(sourceData.data(), fs.size());
	fs.close();

	ShaderProgram *shader = new ShaderProgram();
	shader->type = type;
	shader->loadFromSource(sourceData.data(), sourceData.size());

	m_shaderCache.insert(name, shader);

	return shader;

	/*
	std::ifstream fs(source.cstr());
	std::ostringstream fsBuffer;
	fsBuffer << fs.rdbuf();
	std::string sourceData = fsBuffer.str();

	ShaderProgram *shader = new ShaderProgram();
	shader->type = type;
	shader->loadFromSource(sourceData.data(), sourceData.size());

	m_shaderCache.insert(name, shader);

	return shader;
	*/
}

ShaderEffect *ShaderMgr::createEffect()
{
	ShaderEffect *effect = new ShaderEffect();
	m_effects.pushBack(effect);
	return effect;
}
