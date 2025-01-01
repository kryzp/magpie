#include "shader_mgr.h"
#include "backend.h"
#include "../io/file_stream.h"

#include <fstream>
#include <sstream>
#include <string>

llt::ShaderMgr* llt::g_shaderManager = nullptr;

using namespace llt;

ShaderMgr::ShaderMgr()
	: m_shaderCache()
	, m_effects()
{
}

ShaderMgr::~ShaderMgr()
{
	for (auto& effect : m_effects) {
		delete effect;
	}

	m_effects.clear();

	for (auto& [name, shader] : m_shaderCache) {
		delete shader;
	}

	m_shaderCache.clear();
}

ShaderProgram* ShaderMgr::get(const String& name)
{
	if (m_shaderCache.contains(name)) {
		return m_shaderCache.get(name);
	}

	return nullptr;
}

ShaderProgram* ShaderMgr::create(const String& name, const String& source, VkShaderStageFlagBits type)
{
	if (m_shaderCache.contains(name)) {
		return m_shaderCache.get(name);
	}

	/*
	FileStream fs(source.cstr(), "r");
	Vector<char> sourceData(fs.size());
	fs.read(sourceData.data(), fs.size());
	fs.close();

	ShaderProgram* shader = new ShaderProgram();
	shader->type = type;
	shader->loadFromSource(sourceData.data(), sourceData.size());

	m_shaderCache.insert(name, shader);

	return shader;
	*/

	std::ifstream fs(source.cstr());
	std::ostringstream fsBuffer;
	fsBuffer << fs.rdbuf();
	std::string sourceData = fsBuffer.str();

	ShaderProgram* shader = new ShaderProgram();
	shader->type = type;
	shader->loadFromSource(sourceData.data(), sourceData.size());

	m_shaderCache.insert(name, shader);

	return shader;
}

ShaderEffect* ShaderMgr::createEffect()
{
	ShaderEffect* effect = new ShaderEffect();
	m_effects.pushBack(effect);
	return effect;
}
