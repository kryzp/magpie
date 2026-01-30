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

		VkImageViewType get_default_view_type() const
		{
			if (is_cubemap_texture) {
				if (layer_count > 6)
					return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
				return VK_IMAGE_VIEW_TYPE_CUBE;
			}

			switch (type) {
				case VK_IMAGE_TYPE_1D:
					return layer_count > 1
						? VK_IMAGE_VIEW_TYPE_1D_ARRAY
						: VK_IMAGE_VIEW_TYPE_1D;

				case VK_IMAGE_TYPE_2D:
					return layer_count > 1
						? VK_IMAGE_VIEW_TYPE_2D_ARRAY
						: VK_IMAGE_VIEW_TYPE_2D;

				case VK_IMAGE_TYPE_3D:
					return VK_IMAGE_VIEW_TYPE_3D;
			}

			debug_log_crash("Failed to find default view type for texture of type: %d\n", type);

			return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		}

	private:
		VkImage handle;
		VkImageUsageFlags usage;

		VmaAllocation allocation;
		VmaAllocationInfo allocation_info;

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
	
	struct SubresourceRange {
		constexpr static u32 REMAINING_COUNT = -1u;

		VkImageAspectFlags aspects;
		u32 base_mip;
		u32 mips;
		u32 base_layer;
		u32 layers;

		SubresourceRange of_texture(const Texture *texture) const
		{
			SubresourceRange range = *this;
			range.mips   = mips   == REMAINING_COUNT ? texture->get_mipmap_count() - range.base_mip   : range.mips;
			range.layers = layers == REMAINING_COUNT ? texture->get_layer_count()  - range.base_layer : range.layers;

			return range;
		}

		static SubresourceRange all(VkImageAspectFlags aspects)
		{
			return {
				aspects,
				0, REMAINING_COUNT,
				0, REMAINING_COUNT
			};
		}

		static SubresourceRange all_colour()
		{
			return {
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, REMAINING_COUNT,
				0, REMAINING_COUNT
			};
		}

		static SubresourceRange all_depth()
		{
			return {
				VK_IMAGE_ASPECT_DEPTH_BIT,
				0, REMAINING_COUNT,
				0, REMAINING_COUNT
			};
		}
	};

	class TextureView {
		friend class Device;

	public:
		TextureView()
			: handle()
			, type()
			, range()
			, bindless_handle_sampled()
			, bindless_handle_storage()
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

		const SubresourceRange &get_subresource_range() const
		{
			return range;
		}

		BindlessHandle get_bindless_sampled() const
		{
			return bindless_handle_sampled;
		}

		BindlessHandle get_bindless_storage() const
		{
			return bindless_handle_storage;
		}

	private:
		VkImageView handle;

		VkImageViewType type;
		SubresourceRange range;

		BindlessHandle bindless_handle_sampled;
		BindlessHandle bindless_handle_storage;
	};
}
