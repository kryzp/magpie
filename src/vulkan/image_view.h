#pragma once

#include "third_party/volk.h"

#include "bindless.h"

namespace mgp
{
	class VulkanCore;

	class ImageView
	{
	public:
		ImageView() = default;
		ImageView(VulkanCore *core, VkImageView view, VkFormat format, VkImageUsageFlags usage);

		~ImageView();

		const VkImageView &getHandle() const;
		const VkFormat &getFormat() const;

		bindless::Handle getBindlessHandle() const;

	private:
		VkImageView m_view;
		VkFormat m_format;

		bindless::Handle m_bindlessHandle;

		VulkanCore *m_core;
	};
}
