#ifndef RENDER_IMAGE_H
#define RENDER_IMAGE_H

#include "core/types.h"
#include "graphics/resource_key.h"
#include "graphics/ring_buffer.h"
#include "graphics/cmd_buffer.h"

#include <memory>
#include "container/vector.h"

namespace rnd {

class Image {
public:
	~Image();

	Image(const Image &) = delete;
	Image &operator=(const Image &) = delete;

	// blocks until the pixels are on the GPU. pixels may be null (transparent).
	static Unique<Image> Create(unsigned width, unsigned height, const u8 *rgba);

	static Unique<Image> CreateBlank(unsigned width, unsigned height, VkFormat format, unsigned bytes_per_pixel);

	static Unique<Image> CreateSolid(unsigned width, unsigned height, unsigned rgba);

	static Unique<Image> Wrap(gfx::ResourceKey texture);

	[[nodiscard]] gfx::ResourceKey GetTexture() const;
	[[nodiscard]] gfx::ResourceKey GetView() const;
	[[nodiscard]] unsigned GetBindless() const;

	[[nodiscard]] unsigned GetWidth() const;
	[[nodiscard]] unsigned GetHeight() const;

	[[nodiscard]] u64 ImGuiTextureID() const; // 0 if no imgui

private:
	Image(gfx::ResourceKey texture, bool owned);

	static Unique<Image> CreateWith(unsigned width, unsigned height, VkFormat format, const u8 *data, u64 size);

	gfx::ResourceKey texture;
	gfx::ResourceKey view;
	unsigned bindless;
	unsigned width;
	unsigned height;
	bool owned;
};

class ImageUploads {
public:
	ImageUploads() = default;
	~ImageUploads() = default;

	void Queue(const Image *image, unsigned x, unsigned y, unsigned width, unsigned height, const u8 *rgba);
	void Record(const gfx::CmdBuffer &cmd, gfx::RingBuffer *upload);

	[[nodiscard]] bool IsEmpty() const;

private:
	struct PendingUpload {
		const Image *image;
		unsigned x;
		unsigned y;
		unsigned width;
		unsigned height;
		u64 offset;
	};

	Vector<PendingUpload> pending;
	Vector<u8> bytes;
};

}

#endif // RENDER_IMAGE_H
