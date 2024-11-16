#include "gpu_buffer.h"
#include "texture.h"
#include "backend.h"
#include "util.h"

#include "../common.h"

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

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = m_usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = m_memoryUsage;
	vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	if (VkResult result = vmaCreateBuffer(g_vulkanBackend->m_vmaAllocator, &bufferCreateInfo, &vmaAllocInfo, &m_buffer, &m_allocation, &m_allocationInfo); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN:BUFFER|DEBUG] Failed to create buffer: %d", result);
	}
}

void GPUBuffer::cleanUp()
{
    if (m_buffer == VK_NULL_HANDLE) {
        return;
    }

	vmaDestroyBuffer(g_vulkanBackend->m_vmaAllocator, m_buffer, m_allocation);

    m_buffer = VK_NULL_HANDLE;
}

void GPUBuffer::readDataFromMe(void* dst, uint64_t length, uint64_t offset) const
{
	vmaCopyAllocationToMemory(g_vulkanBackend->m_vmaAllocator, m_allocation, offset, dst, length);
}

void GPUBuffer::writeDataToMe(const void* src, uint64_t length, uint64_t offset) const
{
	vmaCopyMemoryToAllocation(g_vulkanBackend->m_vmaAllocator, src, m_allocation, offset, length);
}

void GPUBuffer::writeToBuffer(const GPUBuffer* other, uint64_t length, uint64_t srcOffset, uint64_t dstOffset)
{
	VkCommandBuffer cmdBuffer = vkutil::beginSingleTimeCommands(g_vulkanBackend->graphicsQueue.getCurrentFrame().commandPool);
	{
		VkBufferCopy region = {};
		region.srcOffset = srcOffset;
		region.dstOffset = dstOffset;
		region.size = length;

		vkCmdCopyBuffer(
			cmdBuffer,
			m_buffer,
			static_cast<const GPUBuffer*>(other)->m_buffer,
			1,
			&region
		);
	}
	vkutil::endSingleTimeGraphicsCommands(cmdBuffer);
}

void GPUBuffer::writeToTexture(const Texture* texture, uint64_t size, uint64_t offset, uint32_t baseArrayLayer)
{
	VkCommandBuffer cmdBuffer = vkutil::beginSingleTimeCommands(g_vulkanBackend->graphicsQueue.getCurrentFrame().commandPool);
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
		region.imageExtent = { texture->width(), texture->height(), 1 };

		vkCmdCopyBufferToImage(
			cmdBuffer,
			m_buffer,
			texture->getImage(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);
	}
	vkutil::endSingleTimeGraphicsCommands(cmdBuffer);

	if (baseArrayLayer == texture->getLayerCount() - 1 && texture->isMipmapped()) {
		texture->generateMipmaps();
	}
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
