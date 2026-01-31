#pragma once

#include <functional>

#include "core/types.h"

#include "device.h"
#include "gpu_buffer.h"
#include "queue.h"

namespace gfx
{
	template <typename T>
	class PerFrame {
	public:
		PerFrame()
			: device(nullptr)
			, data{}
		{
		}

		~PerFrame()
		{
		}

		void init(Device *device, const std::function<T(void)> &creator)
		{
			this->device = device;

			for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
				data[i] = creator();
		}

		T &get()
		{
			return data[device->get_current_frame_index()];
		}

		const T &get() const
		{
			return data[device->get_current_frame_index()];
		}

		T *begin()
		{
			return data;
		}

		T *end()
		{
			return data + FRAMES_IN_FLIGHT;
		}

	private:
		Device *device;
		T data[FRAMES_IN_FLIGHT];
	};

	struct GpuArenaAlloc {
		u64 offset;
		uptr cpu;
		u64 gpu;
	};

	/*
	 * GPU-based Memory Arena (linear allocator).
	 * God I love memory arenas.
	 */
	class GpuArena {
	public:
		GpuArena();
		~GpuArena();

		void allocate(Device *device, VkBufferUsageFlags2 usage, VmaAllocationCreateFlags flags, u64 size);
		void destroy() const;
		
		void reset();
		GpuArenaAlloc push(u64 size, u64 alignment = 16);

		GpuBuffer *get_buffer();
		const GpuBuffer *get_buffer() const;

		uptr base_cpu(uptr offset) const;
		u64 base_gpu(u64 offset) const;

	private:
		Device *device;
		GpuBuffer *buffer;
		u64 used;
	};
}
