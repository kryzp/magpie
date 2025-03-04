#include "gpu_buffer.h"

#include "core/common.h"

#include "texture.h"
#include "backend.h"
#include "util.h"

using namespace llt;

GPUBuffer::GPUBuffer(VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
	: m_buffer(VK_NULL_HANDLE)
	, m_allocation()
	, m_allocationInfo()
	, m_usage(usage)
	, m_memoryUsage(memoryUsage)
	, m_size(0)
{
}

GPUBuffer::~GPUBuffer()
{
    cleanUp();
}

void GPUBuffer::create(uint64_t size)
{
	this->m_size = size;

	// todo
//	Vector<uint32_t> queueFamilyIndices = {
//		g_vulkanBackend->m_graphicsQueue.getFamilyIdx().value(),
//		g_vulkanBackend->m_transferQueues[0].getFamilyIdx().value()
//	};

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = m_usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;//queueFamilyIndices.size();
	bufferCreateInfo.pQueueFamilyIndices = nullptr;//queueFamilyIndices.data();

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = m_memoryUsage;
	vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	LLT_VK_CHECK(
		vmaCreateBuffer(g_vulkanBackend->m_vmaAllocator, &bufferCreateInfo, &vmaAllocInfo, &m_buffer, &m_allocation, &m_allocationInfo),
		"Failed to create buffer"
	);
}

void GPUBuffer::cleanUp()
{
    if (m_buffer == VK_NULL_HANDLE) {
        return;
    }

	vmaDestroyBuffer(g_vulkanBackend->m_vmaAllocator, m_buffer, m_allocation);

    m_buffer = VK_NULL_HANDLE;
}

void GPUBuffer::readDataFromMe(void *dst, uint64_t length, uint64_t offset) const
{
	vmaCopyAllocationToMemory(g_vulkanBackend->m_vmaAllocator, m_allocation, offset, dst, length);
}

void GPUBuffer::writeDataToMe(const void *src, uint64_t length, uint64_t offset) const
{
	vmaCopyMemoryToAllocation(g_vulkanBackend->m_vmaAllocator, src, m_allocation, offset, length);
}

void GPUBuffer::writeToBuffer(const GPUBuffer *other, uint64_t length, uint64_t srcOffset, uint64_t dstOffset)
{
	CommandBuffer commandBuffer = vkutil::beginSingleTimeCommands(g_vulkanBackend->getTransferCommandPool());

	VkBufferCopy region = {};
	region.srcOffset = srcOffset;
	region.dstOffset = dstOffset;
	region.size = length;

	commandBuffer.copyBufferToBuffer(
		m_buffer, other->m_buffer,
		1, &region
	);

	vkutil::endSingleTimeTransferCommands(commandBuffer);
}

void GPUBuffer::writeToTextureSingle(const Texture *texture, uint64_t size, uint64_t offset, uint32_t baseArrayLayer)
{
	CommandBuffer commandBuffer = vkutil::beginSingleTimeCommands(g_vulkanBackend->getTransferCommandPool());
	writeToTexture(commandBuffer, texture, size, offset, baseArrayLayer);
	vkutil::endSingleTimeTransferCommands(commandBuffer);
}

void GPUBuffer::writeToTexture(CommandBuffer &commandBuffer, const Texture *texture, uint64_t size, uint64_t offset, uint32_t baseArrayLayer)
{
	VkBufferImageCopy region = {};
	region.bufferOffset = offset;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = baseArrayLayer;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { texture->getWidth(), texture->getHeight(), 1 };

	commandBuffer.copyBufferToImage(
		m_buffer, texture->getImage(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &region
	);
}

VkBuffer GPUBuffer::getBuffer() const
{
	return m_buffer;
}

VkBufferUsageFlags GPUBuffer::getUsage() const
{
	return m_usage;
}

VmaMemoryUsage GPUBuffer::getMemoryUsage() const
{
	return m_memoryUsage;
}

uint64_t GPUBuffer::getSize() const
{
	return m_size;
}
