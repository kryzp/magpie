#pragma once

#include "core/types.h"

#include "gpu_buffer.h"

namespace gfx
{
	class Device;

	template <typename T>
	struct GpuAlloc {
		u64 offset;
		u64 size;
		T *cpu;
		u64 gpu;
	};

	class GpuRingBuffer {
	public:
		GpuRingBuffer();
		~GpuRingBuffer();

		void allocate(Device *device, VkBufferUsageFlags2 usage, VmaAllocationCreateFlags flags, u64 size);
		void destroy() const;
		
		void reset();

		GpuAlloc<u8> push(u64 bytes, u64 alignment = 16);

		template <typename T>
		GpuAlloc<T> push(u64 count = 1);

		GpuBuffer *get_buffer();
		const GpuBuffer *get_buffer() const;

	private:
		uptr cpu_addr(uptr offset) const;
		u64 gpu_addr(u64 offset) const;

		Device *device;
		GpuBuffer *buffer;
		u64 used;
	};

	template <typename T>
	GpuAlloc<T> GpuRingBuffer::push(u64 count)
	{
		GpuAlloc<u8> bytes = push(sizeof(T) * count, alignof(T));
		
		GpuAlloc<T> alloc = {};
		alloc.offset = bytes.offset;
		alloc.size = bytes.size;
		alloc.cpu = (T *)bytes.cpu;
		alloc.gpu = bytes.gpu;

		return alloc;
	}
}
