#include "render/image.h"
#include "graphics/device.h"
#include "graphics/sync.h"

#include <string.h>

rnd::Image::Image(gfx::ResourceKey texture_, bool owned_)
	: texture(texture_)
	, bindless(0)
	, width(0)
	, height(0)
	, owned(owned_)
{
	gfx::Device *device = gfx::Device::Get();

	const gfx::Texture *gfx_texture = device->TextureFromKey(texture);

	width = gfx_texture->width;
	height = gfx_texture->height;

	view = device->TextureViewAuto(texture);
	bindless = device->TextureViewBindless(view);
}

rnd::Image::~Image()
{
	gfx::Device::Get()->ImGuiForgetTextureView(view);

	if (owned)
		gfx::Device::Get()->TextureDestroy(texture);
}

u64 rnd::Image::ImGuiTextureID() const
{
	return gfx::Device::Get()->ImGuiTextureIdForView(view);
}

Unique<rnd::Image> rnd::Image::Create(unsigned width, unsigned height, const u8 *rgba)
{
	u64 size = (u64)width * (u64)height * 4;

	if (rgba)
		return CreateWith(width, height, VK_FORMAT_R8G8B8A8_UNORM, rgba, size);

	Vector<u8> zeroes(size, 0);
	return CreateWith(width, height, VK_FORMAT_R8G8B8A8_UNORM, zeroes.data(), size);
}

Unique<rnd::Image> rnd::Image::CreateBlank(unsigned width, unsigned height, VkFormat format, unsigned bytes_per_pixel)
{
	u64 size = (u64)width * (u64)height * bytes_per_pixel;

	Vector<u8> zeroes(size, 0);
	return CreateWith(width, height, format, zeroes.data(), size);
}

Unique<rnd::Image> rnd::Image::CreateWith(unsigned width, unsigned height, VkFormat format, const u8 *data, u64 size)
{
	gfx::Device *device = gfx::Device::Get();

	gfx::ResourceKey texture = device->TextureAlloc2D(width, height, format, 1);
	gfx::Texture *gfx_texture = device->TextureFromKey(texture);

	gfx::ResourceKey stage = device->StageAlloc(size);
	device->BufferWrite(stage, data, size, 0);

	gfx::AccessState none = { VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE };
	gfx::AccessState copy = { VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };
	gfx::AccessState shader_read = { VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT };

	gfx::CmdBuffer cmd = device->SubmitImBegin();

	VkImageMemoryBarrier2 to_copy = gfx::sync::TextureBarrier(
		gfx_texture,
		none,
		copy,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		0, VK_REMAINING_MIP_LEVELS,
		0, VK_REMAINING_ARRAY_LAYERS
	);

	cmd.PipelineBarrier(
		0,
		0, nullptr,
		0, nullptr,
		1, &to_copy
	);

	cmd.CopyBufferToTextureWhole(stage, texture, 0);

	VkImageMemoryBarrier2 to_read = gfx::sync::TextureBarrier(
		gfx_texture,
		copy,
		shader_read,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		0, VK_REMAINING_MIP_LEVELS,
		0, VK_REMAINING_ARRAY_LAYERS
	);

	cmd.PipelineBarrier(
		0,
		0, nullptr,
		0, nullptr,
		1, &to_read
	);

	device->SubmitImEnd(cmd);

	device->BufferDestroy(stage);

	return Unique<Image>(new Image(texture, true));
}

Unique<rnd::Image> rnd::Image::CreateSolid(unsigned width, unsigned height, unsigned rgba)
{
	Vector<u8> pixels(width * height * 4);

	for (u64 i = 0; i < width * height; i++) {
		pixels[i*4 + 0] = (u8)((rgba >> 24) & 0xFF);
		pixels[i*4 + 1] = (u8)((rgba >> 16) & 0xFF);
		pixels[i*4 + 2] = (u8)((rgba >>  8) & 0xFF);
		pixels[i*4 + 3] = (u8)((rgba      ) & 0xFF);
	}

	return Create(width, height, pixels.data());
}

Unique<rnd::Image> rnd::Image::Wrap(gfx::ResourceKey texture)
{
	return Unique<Image>(new Image(texture, false));
}

gfx::ResourceKey rnd::Image::GetTexture() const
{
	return texture;
}

gfx::ResourceKey rnd::Image::GetView() const
{
	return view;
}

unsigned rnd::Image::GetBindless() const
{
	return bindless;
}

unsigned rnd::Image::GetWidth() const
{
	return width;
}

unsigned rnd::Image::GetHeight() const
{
	return height;
}

void rnd::ImageUploads::Queue(const Image *image, unsigned x, unsigned y, unsigned width, unsigned height, const u8 *rgba)
{
	PendingUpload p;
	p.image = image;
	p.x = x;
	p.y = y;
	p.width = width;
	p.height = height;
	p.offset = bytes.size();

	u64 size = (u64)width * height * 4;

	bytes.resize(bytes.size() + size);
	memcpy(bytes.data() + p.offset, rgba, size);

	pending.push_back(p);
}

void rnd::ImageUploads::Record(const gfx::CmdBuffer &cmd, gfx::RingBuffer *upload)
{
	if (pending.empty())
		return;

	gfx::Device *device = gfx::Device::Get();

	gfx::AccessState shader_read = { VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT };
	gfx::AccessState copy = { VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };

	for (const PendingUpload &p : pending) {
		u64 size = (u64)p.width * p.height * 4;

		gfx::Alloc alloc = upload->PushCopy(bytes.data() + p.offset, size, 16);

		if (!alloc.cpu)
			continue;

		gfx::Texture *gfx_texture = device->TextureFromKey(p.image->GetTexture());

		VkImageMemoryBarrier2 to_copy = gfx::sync::TextureBarrier(
			gfx_texture,
			shader_read,
			copy,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_GENERAL,
			0, VK_REMAINING_MIP_LEVELS,
			0, VK_REMAINING_ARRAY_LAYERS
		);

		cmd.PipelineBarrier(
			0,
			0, nullptr,
			0, nullptr,
			1, &to_copy
		);

		gfx::BufferImageCopy region;
		region.buffer_offset = alloc.offset;
		region.image_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.image_subresource.mipLevel = 0;
		region.image_subresource.baseArrayLayer = 0;
		region.image_subresource.layerCount = 1;
		region.x = p.x;
		region.y = p.y;
		region.width = p.width;
		region.height = p.height;
		region.depth = 1;

		cmd.CopyBufferToTexture(upload->buffer, p.image->GetTexture(), 1, &region);

		VkImageMemoryBarrier2 to_read = gfx::sync::TextureBarrier(
			gfx_texture,
			copy,
			shader_read,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_GENERAL,
			0, VK_REMAINING_MIP_LEVELS,
			0, VK_REMAINING_ARRAY_LAYERS
		);

		cmd.PipelineBarrier(0, 0, nullptr, 0, nullptr, 1, &to_read);
	}

	pending.clear();
	bytes.clear();
}

bool rnd::ImageUploads::IsEmpty() const
{
	return pending.empty();
}
