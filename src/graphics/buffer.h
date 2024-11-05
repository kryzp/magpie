#ifndef VK_BUFFER_H_
#define VK_BUFFER_H_

#include <vulkan/vulkan.h>

#include "../common.h"

namespace llt
{
	class Texture;
	class VulkanBackend;

	class Buffer
	{
	public:
		Buffer(VkBufferUsageFlags usage);
		~Buffer();

		void create(VkMemoryPropertyFlags properties, uint64_t size);
		void cleanUp();

		void readDataFromMe(void* dst, uint64_t length, uint64_t offset);
		void writeDataToMe(const void* src, uint64_t length, uint64_t offset);

		void writeToBuffer(const Buffer* other, uint64_t length, uint64_t srcOffset, uint64_t dstOffset);
		void writeToTexture(const Texture* texture, uint64_t size, uint64_t offset = 0, uint32_t baseArrayLayer = 0);

		VkBuffer getBuffer() const;
		VkDeviceMemory getMemory() const;

		VkBufferUsageFlags getUsage() const;
		VkMemoryPropertyFlags getProperties() const;

		uint64_t getSize() const;

	private:
		VkBuffer m_buffer;
		VkDeviceMemory m_memory;

		VkBufferUsageFlags m_usage;
		VkMemoryPropertyFlags m_properties;

		uint64_t m_size;
	};
}

#endif // VK_BUFFER_H_
