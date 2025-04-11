#include "gpu_buffer.h"

#include "core/common.h"

#include "core.h"
#include "command_buffer.h"
#include "image.h"

using namespace mgp;

GPUBuffer::GPUBuffer(VulkanCore *core, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, uint64_t size)
	: m_buffer(VK_NULL_HANDLE)
	, m_allocation()
	, m_allocationInfo()
	, m_usage(usage)
	, m_memoryUsage(memoryUsage)
	, m_size(size)
	, m_bindlessHandle(bindless::INVALID_HANDLE)
	, m_core(core)
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = m_size;
	bufferCreateInfo.usage = m_usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;//queueFamilyIndices.size();
	bufferCreateInfo.pQueueFamilyIndices = nullptr;//queueFamilyIndices.data();

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = m_memoryUsage;
	vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	MGP_VK_CHECK(
		vmaCreateBuffer(m_core->getVMAAllocator(), &bufferCreateInfo, &vmaAllocInfo, &m_buffer, &m_allocation, &m_allocationInfo),
		"Failed to create buffer"
	);

	if (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
		m_bindlessHandle = m_core->getBindlessResources().registerBuffer(*this);
}

GPUBuffer::~GPUBuffer()
{
	vmaDestroyBuffer(m_core->getVMAAllocator(), m_buffer, m_allocation);
	m_buffer = VK_NULL_HANDLE;
}

void GPUBuffer::read(void *dst, uint64_t length, uint64_t offset) const
{
	vmaCopyAllocationToMemory(m_core->getVMAAllocator(), m_allocation, offset, dst, length);
}

void GPUBuffer::write(const void *src, uint64_t length, uint64_t offset) const
{
	vmaCopyMemoryToAllocation(m_core->getVMAAllocator(), src, m_allocation, offset, length);
}

VkDescriptorBufferInfo GPUBuffer::getDescriptorInfo(uint32_t offset) const
{
	return {
		.buffer = m_buffer,
		.offset = offset,
		.range = m_size
	};
}

VkDescriptorBufferInfo GPUBuffer::getDescriptorInfoRange(uint32_t range, uint32_t offset) const
{
	return {
		.buffer = m_buffer,
		.offset = offset,
		.range = range
	};
}

const VkBuffer &GPUBuffer::getHandle() const
{
	return m_buffer;
}

VkBufferUsageFlags GPUBuffer::getUsage() const
{
	return m_usage;
}

bool GPUBuffer::isUniform() const
{
	return m_usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
}

bool GPUBuffer::isStorage() const
{
	return m_usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
}

VmaMemoryUsage GPUBuffer::getMemoryUsage() const
{
	return m_memoryUsage;
}

uint64_t GPUBuffer::getSize() const
{
	return m_size;
}

bindless::Handle GPUBuffer::getBindlessHandle() const
{
	return m_bindlessHandle;
}
