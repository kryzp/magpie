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

		void read(void *dst, uint64_t length, uint64_t offset) const;
		void write(const void *src, uint64_t length, uint64_t offset) const;

//		template <typename T>
//		void writeStruct(const T &data, uint64_t index) const;

		VkDescriptorBufferInfo getDescriptorInfo(uint32_t offset = 0) const;
		VkDescriptorBufferInfo getDescriptorInfoRange(uint32_t range, uint32_t offset = 0) const;

		const VkBuffer &getHandle() const;

		VkBufferUsageFlags getUsage() const;

		bool isUniform() const;
		bool isStorage() const;

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
