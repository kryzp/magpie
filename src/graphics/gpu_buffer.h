#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include "core/types.h"

namespace gfx
{
	class GpuBuffer {
		friend class Device;

	public:
		GpuBuffer();
		~GpuBuffer();

		void read(void *dst, u64 length, u64 offset);
		void write(const void *src, u64 length, u64 offset);

		void *map() const;

		bool is_storage() const;
		bool is_uniform() const;

		const VkBuffer &get_handle() const
		{
			return handle;
		}

		VkBufferUsageFlags2 get_usage() const
		{
			return usage;
		}

		u64 get_size() const
		{
			return size;
		}

		VkDeviceAddress get_device_address() const
		{
			return device_address;
		}

		const VmaAllocation &get_allocation() const
		{
			return allocation;
		}

		const VmaAllocationInfo &get_allocation_info() const
		{
			return allocation_info;
		}

		VmaAllocationCreateFlagBits get_allocation_flags() const
		{
			return allocation_flags;
		}

	private:
		VkBuffer handle;
		VkBufferUsageFlags2 usage;

		u64 size;

		VkDeviceAddress device_address;

		VmaAllocator *allocator;
		VmaAllocation allocation;
		VmaAllocationInfo allocation_info;
		VmaAllocationCreateFlagBits allocation_flags;
	};
}
