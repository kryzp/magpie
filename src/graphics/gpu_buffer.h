#pragma once

#include <inttypes.h>

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

namespace mgp
{
	class GraphicsCore;

	class GPUBuffer
	{
	public:
		GPUBuffer(GraphicsCore *gfx, VkBufferUsageFlags usage, VmaAllocationCreateFlagBits flags, uint64_t size);
		~GPUBuffer();

		void read(void *dst, uint64_t length, uint64_t offset) const;
		void write(const void *src, uint64_t length, uint64_t offset) const;

		template <typename T>
		void writeType(const T &t, uint64_t arrayIndex = 0) const
		{
			write(&t, sizeof(T), sizeof(T) * arrayIndex);
		}

		bool isUniformBuffer() const;
		bool isStorageBuffer() const;

		VkBufferUsageFlags getUsage() const;
		VmaAllocationCreateFlagBits getFlags() const;
		
		VkDeviceAddress getDeviceAddress() const;
		uint64_t getSize() const;

		const VkBuffer &getHandle() const;

		VkDescriptorBufferInfo getDescriptorInfo(uint32_t offset = 0) const;
		VkDescriptorBufferInfo getDescriptorInfoRange(uint32_t range, uint32_t offset = 0) const;

	private:
		GraphicsCore *m_gfx;

		VkBuffer m_buffer;

		VmaAllocation m_allocation;
		VmaAllocationInfo m_allocationInfo;

		VkBufferUsageFlags m_usage;
		VmaAllocationCreateFlagBits m_flags;

		VkDeviceAddress m_gpuAddress;

		uint64_t m_size;
	};
}
