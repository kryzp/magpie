#pragma once

#include "third_party/volk.h"

#include "bindless.h"
#include "image.h"

namespace mgp
{
	class VulkanCore;

	class ImageView
	{
	public:
		ImageView(
			VulkanCore *core,
			const ImageInfo &info,
			int layerCount,
			int layer,
			int baseMipLevel
		);

		~ImageView();

		const VkImageView &getHandle() const;

		ImageInfo &getInfo();
		const ImageInfo &getInfo() const;

		bindless::Handle getBindlessHandle() const;

	private:
		VkImageView m_view;

		ImageInfo m_imageInfo;

		bindless::Handle m_bindlessHandle;

		VulkanCore *m_core;
	};
}
