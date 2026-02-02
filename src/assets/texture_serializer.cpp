#include "texture_serializer.h"

#include "graphics/sync.h"

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ext/stb_image_write.h"

using namespace ast;

struct TextureLoadData {
	u32 width, height;
	bool is_hdr;
	void *pixels;
};

static AssetLoadResult texture_load(const AssetLoadContext &ctx)
{
	const String &file_path = ctx.system_file_path();

	int width = 0, height = 0, n_channels = 0;
	void *pixels = nullptr;

	u64 unit;
	bool is_hdr = stbi_is_hdr(file_path.c_str());

	if (stbi_is_hdr(file_path.c_str())) {
		pixels = stbi_loadf(file_path.c_str(), &width, &height, &n_channels, STBI_rgb_alpha);
		unit = sizeof(float);
	} else {
		pixels = stbi_load(file_path.c_str(), &width, &height, &n_channels, STBI_rgb_alpha);
		unit = sizeof(u8);
	}

	TextureLoadData *load_data = new TextureLoadData();
	load_data->width = width;
	load_data->height = height;
	load_data->pixels = pixels;
	load_data->is_hdr = is_hdr;

	AssetLoadResult result = {};
	result.data = load_data;
	result.stage_size = width * height * unit * 4; // * 4 because RGBA
	result.failed = pixels == nullptr;

	return result;
}

static Asset *texture_finalize(
	const AssetLoadContext &ctx, const AssetLoadResult &result,
	gfx::Device &device, gfx::CommandBuffer &cmd,
	gfx::GpuBuffer *stage, u64 stage_base
)
{
	TextureLoadData *load_data = (TextureLoadData *)result.data;

	VkFormat format = load_data->is_hdr
		? VK_FORMAT_R32G32B32A32_SFLOAT
		: VK_FORMAT_R8G8B8A8_UNORM;

	gfx::Texture *gfx_texture = device.alloc_texture_2d(load_data->width, load_data->height, format, 1);

	stage->write(load_data->pixels, result.stage_size, stage_base);

	auto copy_barrier = gfx::sync::texture_memory_barrier(
		gfx_texture,
		gfx::sync::get_src_texture_access(gfx::TEXTURE_ACCESS_UNDEFINED),
		gfx::sync::get_dst_texture_access(gfx::TEXTURE_ACCESS_COPY_DST),
		0, gfx_texture->get_mipmap_count(),
		0, gfx_texture->get_layer_count()
	);

	cmd.pipeline_barrier(0, {}, {}, { copy_barrier });
	
	VkBufferImageCopy region = {};
	region.bufferOffset = stage_base;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { load_data->width, load_data->height, 1 };

	cmd.copy_buffer_to_texture(stage, gfx_texture, { region });

	auto blit_barrier = gfx::sync::texture_memory_barrier(
		gfx_texture,
		gfx::sync::get_src_texture_access(gfx::TEXTURE_ACCESS_COPY_DST),
		gfx::sync::get_dst_texture_access(gfx::TEXTURE_ACCESS_BLIT_DST),
		0, gfx_texture->get_mipmap_count(),
		0, gfx_texture->get_layer_count()
	);

	cmd.pipeline_barrier(0, {}, {}, { blit_barrier });
	cmd.generate_mipmaps(gfx_texture);
	
	return new TextureAsset(gfx_texture, device);
}

static void texture_clean_up(void *data)
{
	TextureLoadData *load_data = (TextureLoadData *)data;
	stbi_image_free(load_data->pixels);
	delete load_data;
}

AssetSerializer ast::get_texture_serializer()
{
	AssetSerializer texture_serializer = {};
	texture_serializer.load = texture_load;
	texture_serializer.finalize = texture_finalize;
	texture_serializer.clean_up = texture_clean_up;

	return texture_serializer;
}
