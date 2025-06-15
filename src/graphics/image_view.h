#pragma once

#include <inttypes.h>

#include <unordered_map>

#include <Volk/volk.h>

namespace mgp
{
	class Image;
	class GraphicsCore;

	class ImageView
	{
	public:
		ImageView(
			GraphicsCore *gfx,
			Image *image,
			int layerCount,
			int layer,
			int baseMipLevel
		);

		~ImageView();

		const VkImageView &getHandle() const;

		Image *getImage();
		const Image *getImage() const;

	private:
		GraphicsCore *m_gfx;

		VkImageView m_view;
		Image *m_image; // todo: use a handle here instead
	};

	class ImageViewCache
	{
	public:
		ImageViewCache() = default;
		~ImageViewCache() = default;

		void init(GraphicsCore *gfx);
		void destroy();

		ImageView *fetchStdView(Image *image);

		ImageView *fetchView(
			Image *image,
			int layerCount,
			int layer,
			int baseMipLevel
		);

	private:
		GraphicsCore *m_gfx;
		std::unordered_map<uint64_t, ImageView *> m_viewCache;
	};
}
