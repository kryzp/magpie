#pragma once

#include "third_party/volk.h"
#include "third_party/vk_mem_alloc.h"

#include "bindless.h"

namespace mgp
{
	class CommandBuffer;
	class Image;
	class VulkanCore;

	class GPUBuffer
	{
	public:
		GPUBuffer(VulkanCore *core, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, uint64_t size);
		~GPUBuffer();

		void readDataFromMe(void *dst, uint64_t length, uint64_t offset) const;
		void writeDataToMe(const void *src, uint64_t length, uint64_t offset) const;

		void writeToBuffer(CommandBuffer &cmd, const GPUBuffer *other, uint64_t length, uint64_t srcOffset, uint64_t dstOffset);
		void writeToImage(CommandBuffer &cmd, const Image *image, uint64_t size, uint64_t offset = 0, uint32_t baseArrayLayer = 0);

		VkDescriptorBufferInfo getDescriptorInfo(uint32_t offset = 0) const;
		VkDescriptorBufferInfo getDescriptorInfoRange(uint32_t range, uint32_t offset = 0) const;

		const VkBuffer &getHandle() const;
		VkBufferUsageFlags getUsage() const;

		VmaMemoryUsage getMemoryUsage() const;

		uint64_t getSize() const;

		bindless::Handle getBindlessHandle() const;

	private:
		VkBuffer m_buffer;

		VmaAllocation m_allocation;
		VmaAllocationInfo m_allocationInfo;

		VkBufferUsageFlags m_usage;
		VmaMemoryUsage m_memoryUsage;

		uint64_t m_size;

		bindless::Handle m_bindlessHandle;

		VulkanCore *m_core;
	};
}
