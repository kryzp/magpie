#include "gpu_buffer.h"
#include "texture.h"
#include "backend.h"
#include "util.h"

#include "../common.h"

using namespace llt;

GPUBuffer::GPUBuffer(VkBufferUsageFlags usage)
	: m_buffer(VK_NULL_HANDLE)
	, m_memory(VK_NULL_HANDLE)
	, m_usage(usage)
	, m_properties()
	, m_size(0)
{
}

GPUBuffer::~GPUBuffer()
{
    cleanUp();
}

void GPUBuffer::create(VkMemoryPropertyFlags properties, uint64_t size)
{
	this->m_properties = properties;
	this->m_size = size;

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = m_usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (VkResult result = vkCreateBuffer(g_vulkanBackend->device, &bufferCreateInfo, nullptr, &m_buffer); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN:BUFFER|DEBUG] Failed to create buffer: %d", result);
	}

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(g_vulkanBackend->device, m_buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = vkutil::findMemoryType(g_vulkanBackend->physicalData.device, memoryRequirements.memoryTypeBits, properties);

	if (VkResult result = vkAllocateMemory(g_vulkanBackend->device, &memoryAllocateInfo, nullptr, &m_memory); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN:BUFFER|DEBUG] Failed to reallocate memory for buffer: %d", result);
	}

	// finally bind the memory
	vkBindBufferMemory(g_vulkanBackend->device, m_buffer, m_memory, 0);
}

void GPUBuffer::cleanUp()
{
    if (m_buffer == VK_NULL_HANDLE &&
        m_memory == VK_NULL_HANDLE)
    {
        return;
    }

	vkDestroyBuffer(g_vulkanBackend->device, m_buffer, nullptr);
	vkFreeMemory(g_vulkanBackend->device, m_memory, nullptr);

    m_buffer = VK_NULL_HANDLE;
    m_memory = VK_NULL_HANDLE;
}

// TODO: STOP REMAPPING ALL THE TIME! IT IS VERY INEFFICIENT TO DO!
// ADD SOME FUNCTIONS WHERE THEY MAP, THEN EXIT, THEN WE CALL ANOTHER FUNCTION TO UNMAP
// SO WE CAN DO SOME CALLS BETWEEN THEM, KINDA MAYBE MAPBEGIN() then MAPEND() OR SOMETHING IDK

void GPUBuffer::readDataFromMe(void* dst, uint64_t length, uint64_t offset)
{
	void* src = nullptr;
	vkMapMemory(g_vulkanBackend->device, m_memory, offset, length, 0, &src);
	mem::copy(dst, src, length);
	vkUnmapMemory(g_vulkanBackend->device, m_memory);
}

void GPUBuffer::writeDataToMe(const void* src, uint64_t length, uint64_t offset)
{
	void* dst = nullptr;
	vkMapMemory(g_vulkanBackend->device, m_memory, offset, length, 0, &dst); // we first have to map our memory to the gpu memory before copying data
	mem::copy(dst, src, length);
	vkUnmapMemory(g_vulkanBackend->device, m_memory); // now that we're finished, we can unmap
}

void GPUBuffer::writeToBuffer(const GPUBuffer* other, uint64_t length, uint64_t srcOffset, uint64_t dstOffset)
{
	VkCommandBuffer cmdBuffer = vkutil::beginSingleTimeCommands(g_vulkanBackend->graphicsQueue.getCurrentFrame().commandPool, g_vulkanBackend->device);
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
	VkCommandBuffer cmdBuffer = vkutil::beginSingleTimeCommands(g_vulkanBackend->graphicsQueue.getCurrentFrame().commandPool, g_vulkanBackend->device);
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
			((Texture*)texture)->getImage(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);
	}
	vkutil::endSingleTimeGraphicsCommands(cmdBuffer);

	if (baseArrayLayer == texture->getLayerCount() - 1 && texture->isMipmapped()) {
		((const Texture*)texture)->generateMipmaps();
	}
}

VkBuffer GPUBuffer::getBuffer() const
{
	return m_buffer;
}

VkDeviceMemory GPUBuffer::getMemory() const
{
	return m_memory;
}

VkBufferUsageFlags GPUBuffer::getUsage() const
{
	return m_usage;
}

VkMemoryPropertyFlags GPUBuffer::getProperties() const
{
	return m_properties;
}

uint64_t GPUBuffer::getSize() const
{
	return m_size;
}
