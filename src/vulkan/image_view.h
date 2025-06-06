#pragma once

#include <Volk/volk.h>

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
			Image *parent,
			int layerCount,
			int layer,
			int baseMipLevel
		);

		~ImageView();

		const VkImageView &getHandle() const;

		Image *getImage();
		const Image *getImage() const;

		uint32_t getBindlessHandle() const;

	private:
		VkImageView m_view;

		Image *m_parent; // todo: use a handle here instead

		uint32_t m_bindlessHandle;

		VulkanCore *m_core;
	};
}
