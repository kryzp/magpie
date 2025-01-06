#include "shader_buffer.h"
#include "backend.h"
#include "gpu_buffer_mgr.h"

using namespace llt;

ShaderBuffer::ShaderBuffer()
	: m_buffer(nullptr)
	, m_info()
	, m_dynamicOffset()
	, m_usageInFrame()
	, m_offset(0)
	, m_maxSize(0)
	, m_type(SHADER_BUFFER_NONE)
{
}

void ShaderBuffer::init(uint64_t initialSize, ShaderBufferType type)
{
	m_type = type;
	m_usageInFrame.clear();

	reallocateBuffer(initialSize);
}

void ShaderBuffer::cleanUp()
{
	delete m_buffer;
	m_buffer = nullptr;
}

void ShaderBuffer::pushData(ShaderParameters& params)
{
	auto& packedParams = params.getPackedConstants();
	pushData(packedParams.data(), packedParams.size());
}

void ShaderBuffer::pushData(const void* data, uint64_t size)
{
	if (!data || !size) {
		return;
	}

	// calculate the total used memory so far
	uint64_t totalUsedMemory = 0;
	for (int i = 0; i < m_usageInFrame.size(); i++) {
		totalUsedMemory += m_usageInFrame[i];
	}

	// check if we need to increase in size
	while (totalUsedMemory + size > m_maxSize) {
		reallocateBuffer(m_maxSize * 2);
	}

	// wrap back around to zero if we can't fit all of our data at the current point
	if (m_offset + size >= m_maxSize) {
		m_offset = 0;
	}

	m_dynamicOffset = m_offset;

	m_info.offset = 0;
	m_info.range = size;

	// write the data to the buffer!
	m_buffer->writeDataToMe(data, size, m_offset);

	// move forward and increment the ubo usage in the current frame
	m_offset += size;
	m_usageInFrame[g_vulkanBackend->getCurrentFrameIdx()] += size;
}

void ShaderBuffer::reallocateBuffer(uint64_t size)
{
	delete m_buffer;

	VkDeviceSize bufferSize = vkutil::calcShaderBufferAlignedSize(size);

	m_maxSize = size;
	m_offset = 0;

	if (m_type == SHADER_BUFFER_UBO)
	{
		m_buffer = g_gpuBufferManager->createUniformBuffer(bufferSize);
	}
	else if (m_type == SHADER_BUFFER_SSBO)
	{
		m_buffer = g_gpuBufferManager->createShaderStorageBuffer(bufferSize);
	}
	else
	{
		LLT_ERROR("[SHADERBUFFERMGR|DEBUG] Unsupported ShaderBufferType: %d.", m_type);
	}

	// all of our descriptor sets are invalid now so we have to clear our caches
	g_vulkanBackend->clearDescriptorCacheAndPool();

	m_info.buffer = m_buffer->getBuffer();
	m_info.offset = 0;
	m_info.range = 0;

	if (m_type == SHADER_BUFFER_UBO)
	{
		LLT_LOG("[SHADERBUFFERMGR] (Re)Allocated ubo with size %llu.", size);
	}
	else if (m_type == SHADER_BUFFER_SSBO)
	{
		LLT_LOG("[SHADERBUFFERMGR] (Re)Allocated ssbo with size %llu.", size);
	}
}

void ShaderBuffer::resetBufferUsageInFrame()
{
	m_usageInFrame[g_vulkanBackend->getCurrentFrameIdx()] = 0;
}

const VkDescriptorBufferInfo& ShaderBuffer::getDescriptor() const
{
	return m_info;
}

VkDescriptorType ShaderBuffer::getDescriptorType() const
{
	return m_type == SHADER_BUFFER_UBO ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
}

uint32_t ShaderBuffer::getDynamicOffset() const
{
	return m_dynamicOffset;
}

GPUBuffer* ShaderBuffer::getBuffer()
{
	return m_buffer;
}

const GPUBuffer* ShaderBuffer::getBuffer() const
{
	return m_buffer;
}
