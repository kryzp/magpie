#ifndef VK_BUFFER_H_
#define VK_BUFFER_H_

#include <vulkan/vulkan.h>

#include "../common.h"

namespace llt
{
	class Texture;
	class VulkanBackend;

	enum BufferUsageEnum
	{
		GBUF_USAGE_NONE              = 0 << 0,
		GBUF_USAGE_GPU_TO_CPU        = 1 << 0,
		GBUF_USAGE_CPU_ONLY          = 1 << 1,
		GBUF_USAGE_DETAIL_WRITE_ONLY = 1 << 2,
		GBUF_USAGE_GPU_ONLY          = GBUF_USAGE_GPU_TO_CPU,
		GBUF_USAGE_CPU_TO_GPU        = GBUF_USAGE_CPU_ONLY,
		GBUF_USAGE_MAX
	};

	using BufferUsage = uint8_t; // allow implicit conversation

	class Buffer
	{
	public:
		Buffer(BufferUsage usage);
		~Buffer();

		void create(VkMemoryPropertyFlags properties, uint64_t size);
		void cleanUp();

		void readDataFromMemory(const void* src, uint64_t length, uint64_t offset);
		void writeDataToMemory(void* dst, uint64_t length, uint64_t offset);

		void writeToBuffer(const Buffer* other, uint64_t length, uint64_t srcOffset, uint64_t dstOffset);
		void writeToTexture(const Texture* texture, uint64_t size, uint64_t offset = 0, uint32_t baseArrayLayer = 0);

		VkBuffer getBuffer() const;
		VkDeviceMemory getMemory() const;

		BufferUsage getUsage() const;
		VkMemoryPropertyFlags getProperties() const;

	private:
		VkBuffer m_buffer;
		VkDeviceMemory m_memory;

		BufferUsage m_usage;
		VkMemoryPropertyFlags m_properties;
	};
}

#endif // VK_BUFFER_H_
