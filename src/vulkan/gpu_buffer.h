#pragma once

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include "bindless.h"

namespace mgp
{
	class CommandBuffer;
	class Image;
	class VulkanCore;

	class GPUBuffer
	{
	public:
		GPUBuffer(VulkanCore *core, VkBufferUsageFlags usage, VmaAllocationCreateFlagBits flags, uint64_t size);
		~GPUBuffer();

		void read(void *dst, uint64_t length, uint64_t offset) const;
		void write(const void *src, uint64_t length, uint64_t offset) const;

		template <typename T>
		void writeType(const T &data, uint64_t index = 0) const
		{ write(&data, sizeof(T), sizeof(T) * index); }

		VkDescriptorBufferInfo getDescriptorInfo(uint32_t offset = 0) const;
		VkDescriptorBufferInfo getDescriptorInfoRange(uint32_t range, uint32_t offset = 0) const;

		const VkBuffer &getHandle() const;

		VmaAllocationCreateFlagBits getFlags() const;
		VkBufferUsageFlags getUsage() const;

		VkDeviceAddress getGPUAddress() const;

		bool isUniformBuffer() const;
		bool isStorageBuffer() const;

		uint64_t getSize() const;

		uint32_t getBindlessHandle() const;

	private:
		VkBuffer m_buffer;

		VmaAllocation m_allocation;
		VmaAllocationInfo m_allocationInfo;

		VkBufferUsageFlags m_usage;
		VmaAllocationCreateFlagBits m_flags;

		VkDeviceAddress m_gpuAddress;

		uint64_t m_size;

		uint32_t m_bindlessHandle;

		VulkanCore *m_core;
	};
}
