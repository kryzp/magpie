#include "shader_mgr.h"
#include "backend.h"
#include "../io/file_stream.h"

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

	for (auto& [id, shader] : m_shaderCache) {
		delete shader;
	}

	m_shaderCache.clear();
}

ShaderProgram* ShaderMgr::create(const String& source, VkShaderStageFlagBits type)
{
	uint64_t hash = 0;

	hash::combine(&hash, &source);
	hash::combine(&hash, &type);

	if (m_shaderCache.contains(hash)) {
		return m_shaderCache.get(hash);
	}

	FileStream fs(source.cstr(), "r");
	Vector<char> sourceData(fs.size());
	fs.read(sourceData.data(), fs.size());
	fs.close();

	ShaderProgram* shader = new ShaderProgram();
	shader->type = type;
	shader->loadFromSource(sourceData.data(), sourceData.size());

	m_shaderCache.insert(Pair(hash, shader));

	return shader;
}

ShaderEffect* ShaderMgr::buildEffect()
{
	ShaderEffect* effect = new ShaderEffect();
	m_effects.pushBack(effect);
	return effect;
}
