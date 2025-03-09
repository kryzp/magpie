#include "shader_buffer.h"

#include "vulkan/backend.h"

#include "rendering/gpu_buffer_mgr.h"

using namespace llt;

DynamicShaderBuffer::DynamicShaderBuffer()
	: m_buffer(nullptr)
	, m_info()
	, m_dynamicOffset()
	, m_usageInFrame()
	, m_offset(0)
	, m_maxSize(0)
	, m_type(SHADER_BUFFER_MAX_ENUM)
{
}

void DynamicShaderBuffer::init(uint64_t initialSize, ShaderBufferType type)
{
	m_type = type;
	m_usageInFrame.clear();

	reallocateBuffer(initialSize);
}

void DynamicShaderBuffer::cleanUp()
{
	delete m_buffer;

	m_buffer = nullptr;
}

void DynamicShaderBuffer::pushData(const void *data, uint64_t size)
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
		LLT_LOG("Shader buffer expanding size and reallocating...");
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

void DynamicShaderBuffer::reallocateBuffer(uint64_t allocationSize)
{
	delete m_buffer;

	VkDeviceSize bufferSize = vkutil::calcShaderBufferAlignedSize(allocationSize);

	m_maxSize = allocationSize;
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
		LLT_ERROR("Unsupported ShaderBufferType: %d.", m_type);
	}

	m_info.buffer = m_buffer->getBuffer();
	m_info.offset = 0;
	m_info.range = 0;

	if (m_type == SHADER_BUFFER_UBO)
	{
		LLT_LOG("Allocated UBO with allocation size %llu.", allocationSize);
	}
	else if (m_type == SHADER_BUFFER_SSBO)
	{
		LLT_LOG("Allocated SSBO with allocation size %llu.", allocationSize);
	}
}

void DynamicShaderBuffer::resetBufferUsageInFrame()
{
	m_usageInFrame[g_vulkanBackend->getCurrentFrameIdx()] = 0;
}

const VkDescriptorBufferInfo &DynamicShaderBuffer::getDescriptorInfo() const
{
	return m_info;
}

VkDescriptorType DynamicShaderBuffer::getDescriptorType() const
{
	return m_type == SHADER_BUFFER_UBO ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
}

uint32_t DynamicShaderBuffer::getDynamicOffset() const
{
	return m_dynamicOffset;
}

GPUBuffer *DynamicShaderBuffer::getBuffer()
{
	return m_buffer;
}

const GPUBuffer *DynamicShaderBuffer::getBuffer() const
{
	return m_buffer;
}
