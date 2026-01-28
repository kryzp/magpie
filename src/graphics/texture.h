#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include "core/types.h"

#include "bindless.h"

namespace gfx
{
	class Texture {
		friend class Device;

	public:
		Texture()
			: handle()
			, usage()
			, allocation()
			, allocation_info()
//			, access_types{}
			, width()
			, height()
			, depth()
			, is_depth_texture()
			, is_storage_texture()
			, is_cubemap_texture()
			, is_swapchain_texture()
			, format()
			, type()
			, tiling()
			, aspect_count()
			, aspect_flags()
			, layer_count()
			, mipmap_count()
			, sample_count()
		{
		}

		~Texture() = default;

		const VkImage &get_handle() const
		{
			return handle;
		}

		VkImageUsageFlags get_usage() const
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

		u32 get_width() const
		{
			return width;
		}

		u32 get_height() const
		{
			return height;
		}

		u32 get_depth() const
		{
			return depth;
		}

		bool is_transient() const
		{
			return is_transient_texture;
		}

		bool is_depth() const
		{
			return is_depth_texture;
		}

		bool is_cubemap() const
		{
			return is_cubemap_texture;
		}

		bool is_storage() const
		{
			return is_storage_texture;
		}

		bool is_swapchain() const
		{
			return is_swapchain_texture;
		}

		VkFormat get_format() const
		{
			return format;
		}

		VkImageType get_type() const
		{
			return type;
		}

		VkImageTiling get_tiling() const
		{
			return tiling;
		}

		u32 get_aspect_count() const
		{
			return aspect_count;
		}

		VkImageAspectFlags get_aspects() const
		{
			return aspect_flags;
		}

		u32 get_layer_count() const
		{
			return layer_count;
		}

		u32 get_mipmap_count() const
		{
			return mipmap_count;
		}

		VkSampleCountFlags get_sample_count() const
		{
			return sample_count;
		}

	private:
		VkImage handle;
		VkImageUsageFlags usage;

		VmaAllocation allocation;
		VmaAllocationInfo allocation_info;

//		TextureAccessType access_types[16][16][16];

		u32 width;
		u32 height;
		u32 depth;

		// TODO: Convert to flags.
		bool is_transient_texture;
		bool is_depth_texture;
		bool is_storage_texture;
		bool is_cubemap_texture;
		bool is_swapchain_texture;

		VkFormat format;
		VkImageType type;
		VkImageTiling tiling;

		u32 aspect_count;
		VkImageAspectFlags aspect_flags;

		u32 layer_count;
		u32 mipmap_count;

		VkSampleCountFlags sample_count;
	};

	class TextureView {
		friend class Device;

	public:
		TextureView()
			: handle()
			, type()
			, base_layer()
			, layer_count()
			, base_mip()
			, mipmap_count()
			, aspect()
			, bindless()
		{
		}

		~TextureView() = default;

		const VkImageView &get_handle() const
		{
			return handle;
		}

		VkImageViewType get_type() const
		{
			return type;
		}

		u32 get_base_layer() const
		{
			return base_layer;
		}

		u32 get_layer_count() const
		{
			return layer_count;
		}

		u32 get_base_mip() const
		{
			return base_layer;
		}

		u32 get_mipmap_count() const
		{
			return mipmap_count;
		}

		VkImageAspectFlags get_aspect() const
		{
			return aspect;
		}

		const BindlessView &get_bindless() const
		{
			return bindless;
		}

	private:
		VkImageView handle;
		VkImageViewType type;

		u32 base_layer;
		u32 layer_count;

		u32 base_mip;
		u32 mipmap_count;

		VkImageAspectFlags aspect;

		BindlessView bindless;
	};
}
