#include "ubo_manager.h"
#include "backend.h"
#include "buffer_mgr.h"

using namespace llt;

UBOManager::UBOManager()
	: m_ubo(nullptr)
	, m_uboDynamicOffsets()
	, m_uboInfos()
	, m_uboUsageInFrame()
	, m_uboOffset(0)
	, m_uboMaxSize(0)
{
}

void UBOManager::init(uint64_t initialSize)
{
	reallocate_uniform_buffer(initialSize);

	m_uboUsageInFrame.clear();
}

void UBOManager::cleanUp()
{
	delete m_ubo;
	m_ubo = nullptr;
}

void UBOManager::push_data(VkShaderStageFlagBits shader, const void* data, uint64_t size, bool* modified)
{
	// calculate the total used memory so far
	uint64_t totalUsedMemory = 0;
	for (int i = 0; i < m_uboUsageInFrame.size(); i++) {
		totalUsedMemory += m_uboUsageInFrame[i];
	}

	// check if we need to increase in size
	if (totalUsedMemory + size > m_uboMaxSize) {
		reallocate_uniform_buffer(m_uboMaxSize * 2);
	}

	// wrap back around to zero if we can't fit all of our data at the current point
	if (m_uboOffset + size >= m_uboMaxSize) {
		m_uboOffset = 0;
	}

	// calculate the aligned dynamic offset
	uint32_t dynamicOffset = vkutil::calcUboAlignedSize(
		m_uboOffset,
		g_vulkanBackend->physicalData.properties
	);

	// set the dynamic offset
	m_uboDynamicOffsets[shader] = dynamicOffset;

	m_uboInfos[shader].offset = 0;

	if (m_uboInfos[shader].range != size && modified) {
		(*modified) = true;
	}

	m_uboInfos[shader].range = size;

	// actually read the data into the ubo buffer
	m_ubo->readDataFromMemory(data, size, dynamicOffset);

	// move forward and increment the ubo usage in the current frame
	m_uboOffset += size;
	m_uboUsageInFrame[g_vulkanBackend->graphicsQueue.getCurrentFrameIdx()] += size;

	// again, if we have moved past the maximum size, wrap back around to zero.
	if (m_uboOffset >= m_uboMaxSize) {
		m_uboOffset = 0;
	}
}

void UBOManager::reallocate_uniform_buffer(uint64_t size)
{
	delete m_ubo;

	VkDeviceSize bufferSize = vkutil::calcUboAlignedSize(size, g_vulkanBackend->physicalData.properties);

	m_uboMaxSize = size;
	m_uboOffset = 0;

	// recreate the uniform buffer
	m_ubo = g_bufferManager->createUniformBuffer(bufferSize);

	for (auto& info : m_uboInfos) {
		info.buffer = m_ubo->getBuffer();
		info.offset = 0;
		info.range = 0;
	}

	// all of our descriptor sets are invalid now so we have to clear our caches
	g_vulkanBackend->clearDescriptorSetAndPool();

	LLT_LOG("[VULKAN:UBO] (Re)Allocated uniform buffer with size %u.", size);
}

void UBOManager::resetUboUsageInFrame()
{
	m_uboUsageInFrame[g_vulkanBackend->graphicsQueue.getCurrentFrameIdx()] = 0;
}

uint64_t UBOManager::getDescriptorCount() const
{
	for (int i = 0; i < m_uboInfos.size(); i++) {
		if (m_uboInfos[i].range <= 0) {
			return 0;
		}
	}

	return m_uboInfos.size();
}

const VkDescriptorBufferInfo& UBOManager::getDescriptor(uint64_t idx) const
{
	return m_uboInfos[idx];
}

const Array<VkDescriptorBufferInfo, mgc::RASTER_SHADER_COUNT>& UBOManager::getDescriptors() const
{
	return m_uboInfos;
}

const Array<uint32_t, mgc::RASTER_SHADER_COUNT>& UBOManager::getDynamicOffsets() const
{
	return m_uboDynamicOffsets;
}

uint64_t UBOManager::getDynamicOffsetCount() const
{
	for (int i = 0; i < m_uboDynamicOffsets.size(); i++) {
		if (m_uboDynamicOffsets[i] <= 0) {
			return 0;
		}
	}

	return m_uboDynamicOffsets.size();
}
