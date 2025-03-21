#include "shader_buffer_mgr.h"

#include "core/common.h"

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

DynamicShaderBuffer *ShaderBufferMgr::createUBO(uint64_t perFrameSize)
{
	DynamicShaderBuffer *ubo = new DynamicShaderBuffer();
	ubo->init(perFrameSize * mgc::FRAMES_IN_FLIGHT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
	m_buffers.pushBack(ubo);
	return m_buffers.back();
}

DynamicShaderBuffer *ShaderBufferMgr::createSSBO(uint64_t perFrameSize)
{
	DynamicShaderBuffer *ssbo = new DynamicShaderBuffer();
	ssbo->init(perFrameSize * mgc::FRAMES_IN_FLIGHT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
	m_buffers.pushBack(ssbo);
	return m_buffers.back();
}
