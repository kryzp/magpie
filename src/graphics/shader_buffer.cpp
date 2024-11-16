#include "shader_buffer.h"
#include "backend.h"
#include "gpu_buffer_mgr.h"

using namespace llt;

ShaderBuffer::ShaderBuffer()
	: m_buffer(nullptr)
	, m_dynamicOffset()
	, m_info()
	, m_usageInFrame()
	, m_offset(0)
	, m_maxSize(0)
	, m_boundIdx(0)
	, m_isBound(false)
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
	// calculate the total used memory so far
	uint64_t totalUsedMemory = 0;
	for (int i = 0; i < m_usageInFrame.size(); i++) {
		totalUsedMemory += m_usageInFrame[i];
	}

	// check if we need to increase in size
	while (totalUsedMemory + size > m_maxSize) {
		reallocateBuffer(m_maxSize * 2);
	}

	// calculate the aligned dynamic offset
	uint32_t dynamicOffset = vkutil::calcShaderBufferAlignedSize(
		m_offset,
		g_vulkanBackend->physicalData.properties
	);

	// wrap back around to zero if we can't fit all of our data at the current point
	if (dynamicOffset + size >= m_maxSize) {
		m_offset = 0;
	}

	// set the dynamic offset
	m_dynamicOffset = dynamicOffset;

	m_info.offset = 0;
	m_info.range = size;

	if (m_type == SHADER_BUFFER_UBO)
	{
		// actually write the data into the ubo
		m_buffer->writeDataToMe(data, size, dynamicOffset);
	}
	else if (m_type == SHADER_BUFFER_SSBO)
	{
		// actually write the data into the ssbo
		// todo: do all stages have to be created on heap? pretty expensive and they get destroyed like right after, would make more sense to make them on stack...
		// todo: also wait can't we just do writeDataToMe??? fix this!!
		GPUBuffer* stage = g_gpuBufferManager->createStagingBuffer(size);
		stage->writeDataToMe(data, size, 0);
		stage->writeToBuffer(m_buffer, size, 0, dynamicOffset);
		delete stage;
	}
	else
	{
		LLT_ERROR("[VULKAN:SHADERBUFFERMGR|DEBUG] Unsupported ShaderBufferType: %d.", m_type);
	}

	// move forward and increment the ubo usage in the current frame
	m_offset += size;
	m_usageInFrame[g_vulkanBackend->getCurrentFrameIdx()] += size;

	// recalculate the aligned dynamic offset
	dynamicOffset = vkutil::calcShaderBufferAlignedSize(
		m_offset,
		g_vulkanBackend->physicalData.properties
	);

	// again, if we have moved past the maximum size, wrap back around to zero.
	if (dynamicOffset >= m_maxSize) {
		m_offset = 0;
	}
}

void ShaderBuffer::reallocateBuffer(uint64_t size)
{
	delete m_buffer;

	VkDeviceSize bufferSize = vkutil::calcShaderBufferAlignedSize(size, g_vulkanBackend->physicalData.properties);

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
		LLT_ERROR("[VULKAN:SHADERBUFFERMGR|DEBUG] Unsupported ShaderBufferType: %d.", m_type);
	}

	m_info.buffer = m_buffer->getBuffer();
	m_info.offset = 0;
	m_info.range = 0;

	// all of our descriptor sets are invalid now so we have to clear our caches
	g_vulkanBackend->clearDescriptorCacheAndPool();

	if (m_type == SHADER_BUFFER_UBO)
	{
		LLT_LOG("[VULKAN:SHADERBUFFERMGR] (Re)Allocated ubo with size %llu.", size);
	}
	else if (m_type == SHADER_BUFFER_SSBO)
	{
		LLT_LOG("[VULKAN:SHADERBUFFERMGR] (Re)Allocated ssbo with size %llu.", size);
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

void ShaderBuffer::bind(uint32_t idx)
{
	if (m_boundIdx != idx) {
		g_vulkanBackend->markDescriptorDirty();
	}

	m_boundIdx = idx;
	m_isBound = true;
}

void ShaderBuffer::unbind()
{
	m_isBound = false;
}

uint32_t ShaderBuffer::getBoundIdx() const
{
	return m_boundIdx;
}

bool ShaderBuffer::isBound() const
{
	return m_isBound;
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
