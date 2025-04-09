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
			Image *parent,
			int layerCount,
			int layer,
			int baseMipLevel
		);

		~ImageView();

		const VkImageView &getHandle() const;

		Image *getImage();
		const Image *getImage() const;

		bindless::Handle getBindlessHandle() const;

	private:
		VkImageView m_view;

		Image *m_parent; // todo: use a handle here instead

		bindless::Handle m_bindlessHandle;

		VulkanCore *m_core;
	};
}
