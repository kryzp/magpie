#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include "core/types.h"

namespace gfx
{
	class GpuBuffer {
		friend class Device;

	public:
		GpuBuffer()
			: handle()
			, usage()
			, allocator()
			, allocation()
			, allocation_info()
			, allocation_flags()
			, size()
			, device_address()
		{
		}

		~GpuBuffer() = default;

		void read(void *dst, u64 length, u64 offset);
		void write(const void *src, u64 length, u64 offset);

		uptr map() const;

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

		const VmaAllocation &get_allocation() const
		{
			return allocation;
		}

		const VmaAllocationInfo &get_allocation_info() const
		{
			return allocation_info;
		}

		VmaAllocationCreateFlags get_allocation_flags() const
		{
			return allocation_flags;
		}

		u64 get_size() const
		{
			return size;
		}

		u64 get_device_address() const
		{
			return device_address;
		}

	private:
		VkBuffer handle;
		VkBufferUsageFlags2 usage;

		VmaAllocator *allocator;
		VmaAllocation allocation;
		VmaAllocationInfo allocation_info;
		VmaAllocationCreateFlags allocation_flags;

		u64 size;

		VkDeviceAddress device_address;
	};

	/*
	 * Not part of core "Vulkan" but useful as an "equivalent" of texture views.
	 *
	 * In some cases, we might find we want to use the same buffer for multiple purposes.
	 * In those situations, we need a way to refer to subsections of a buffer.
	 */
	class GpuBufferRange {
	public:
		GpuBufferRange(const GpuBuffer *parent, u64 size, u64 offset)
			: parent(parent)
			, size(size)
			, offset(offset)
		{
		}

		~GpuBufferRange()
		{
		}

		const GpuBuffer *get_buffer() const
		{
			return parent;
		}

		u64 get_device_address() const
		{
			return parent->get_device_address() + offset;
		}

		u64 get_size() const
		{
			return size;
		}

		u64 get_offset() const
		{
			return offset;
		}

	private:
		const GpuBuffer *parent;

		u64 size;
		u64 offset;
	};
}
