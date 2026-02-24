#pragma once

#include <volk/volk.h>

#include "core/types.h"
#include "container/vector.h"

#include "texture.h"

namespace gfx
{
	class Texture;
	class TextureView;

	class Swapchain {
		friend class Device;

	public:
		Swapchain()
			: handle()
			, current_texture_index()
			, textures()
			, views()
			, width()
			, height()
			, format()
		{
		}

		~Swapchain() = default;

		const VkSwapchainKHR &get_handle() const
		{
			return handle;
		}

		u32 get_current_texture_index() const
		{
			return current_texture_index;
		}

		Texture *get_current_texture()
		{
			return &textures[current_texture_index];
		}

		const Texture *get_current_texture() const
		{
			return &textures[current_texture_index];
		}

		TextureView *get_current_view()
		{
			return views[current_texture_index];
		}

		const TextureView *get_current_view() const
		{
			return views[current_texture_index];
		}

		u32 get_width() const
		{
			return width;
		}

		u32 get_height() const
		{
			return height;
		}

		VkFormat get_format() const
		{
			return format;
		}

		// TODO: Add recreate(VkExtent2D new_extent) function.

	private:
		VkSwapchainKHR handle;

		// This is *DIFFERENT* from Device::current_frame_index.
		// A swapchain might have, e.g: 3 frames while the graphics
		// device only has 2 frames in flight. They are *usually* the same
		// but not always!
		u32 current_texture_index;

		Vector<Texture> textures;
		Vector<TextureView *> views;

		u32 width;
		u32 height;

		VkFormat format;
	};
}
