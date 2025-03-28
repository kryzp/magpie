#ifndef TEXTURE_VIEW_H_
#define TEXTURE_VIEW_H_

#include "third_party/volk.h"

#include "rendering/types.h"

namespace llt
{
	class Texture;

	class TextureView
	{
		friend class BindlessResourceManager;

	public:
		TextureView() = default;
		TextureView(VkImageView view, VkFormat format);

		~TextureView() = default;

		void cleanUp();

		const VkImageView &getHandle() const;
		const VkFormat &getFormat() const;
		
		BindlessResourceHandle getBindlessHandle() const;

	private:
		VkImageView m_view;
		VkFormat m_format;

		BindlessResourceHandle m_bindlessHandle;
	};
}

#endif // TEXTURE_VIEW_H_
