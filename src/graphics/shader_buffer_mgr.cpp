#include "shader_buffer_mgr.h"
#include "../common.h"

llt::ShaderBufferMgr *llt::g_shaderBufferManager = nullptr;

using namespace llt;

ShaderBufferMgr::ShaderBufferMgr()
	: m_buffers()
{
}

ShaderBufferMgr::~ShaderBufferMgr()
{
	for (auto &buf : m_buffers) {
		buf->cleanUp();
	}
}

void ShaderBufferMgr::resetBufferUsageInFrame()
{
	for (auto &buf : m_buffers) {
		buf->resetBufferUsageInFrame();
	}
}

DynamicShaderBuffer *ShaderBufferMgr::createUBO()
{
	DynamicShaderBuffer *ubo = new DynamicShaderBuffer();
	ubo->init(INIT_BUFFER_SIZE, SHADER_BUFFER_UBO);
	m_buffers.pushBack(ubo);
	return m_buffers.back();
}

DynamicShaderBuffer *ShaderBufferMgr::createSSBO()
{
	DynamicShaderBuffer *ssbo = new DynamicShaderBuffer();
	ssbo->init(INIT_BUFFER_SIZE, SHADER_BUFFER_SSBO);
	m_buffers.pushBack(ssbo);
	return m_buffers.back();
}
