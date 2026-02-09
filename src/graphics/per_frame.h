#pragma once

#include "device.h"

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
}
