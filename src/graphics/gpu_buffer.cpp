#include "gpu_buffer.h"

#include "core/common.h"

#include "graphics_core.h"

using namespace mgp;

GPUBuffer::GPUBuffer(GraphicsCore *gfx, VkBufferUsageFlags usage, VmaAllocationCreateFlagBits flags, uint64_t size)
	: m_gfx(gfx)
	, m_buffer(VK_NULL_HANDLE)
	, m_allocation()
	, m_allocationInfo()
	, m_usage(usage)
	, m_flags(flags)
	, m_gpuAddress(0)
	, m_size(size)
{
	if (isStorageBuffer())
		m_usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT; // enable for all ssbo's

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = m_size;
	bufferCreateInfo.usage = m_usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;//queueFamilyIndices.size();
	bufferCreateInfo.pQueueFamilyIndices = nullptr;//queueFamilyIndices.data();

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	vmaAllocInfo.flags = m_flags | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	mgp_VK_CHECK(
		vmaCreateBuffer(m_gfx->getVMAAllocator(), &bufferCreateInfo, &vmaAllocInfo, &m_buffer, &m_allocation, &m_allocationInfo),
		"Failed to create buffer"
	);

	if (isStorageBuffer())
	{
		VkBufferDeviceAddressInfo addressInfo = {};
		addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		addressInfo.buffer = m_buffer;

		m_gpuAddress = vkGetBufferDeviceAddress(m_gfx->getLogicalDevice(), &addressInfo);
	}
}

GPUBuffer::~GPUBuffer()
{
	vmaDestroyBuffer(m_gfx->getVMAAllocator(), m_buffer, m_allocation);
	m_buffer = VK_NULL_HANDLE;
}

void GPUBuffer::read(void *dst, uint64_t length, uint64_t offset) const
{
	vmaCopyAllocationToMemory(m_gfx->getVMAAllocator(), m_allocation, offset, dst, length);
}

void GPUBuffer::write(const void *src, uint64_t length, uint64_t offset) const
{
	vmaCopyMemoryToAllocation(m_gfx->getVMAAllocator(), src, m_allocation, offset, length);
}

bool GPUBuffer::isUniformBuffer() const
{
	return m_usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
}

bool GPUBuffer::isStorageBuffer() const
{
	return m_usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
}

VkBufferUsageFlags GPUBuffer::getUsage() const
{
	return m_usage;
}

VmaAllocationCreateFlagBits GPUBuffer::getFlags() const
{
	return m_flags;
}

VkDeviceAddress GPUBuffer::getDeviceAddress() const
{
	return m_gpuAddress;
}

uint64_t GPUBuffer::getSize() const
{
	return m_size;
}

const VkBuffer &GPUBuffer::getHandle() const
{
	return m_buffer;
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
