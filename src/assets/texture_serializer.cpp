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

	int width, height, n_channels;
	void *pixels;
	u64 unit;
	
	bool is_hdr = stbi_is_hdr(file_path.c_str());

	if (stbi_is_hdr(file_path.c_str())) {
		pixels = stbi_loadf(file_path.c_str(), &width, &height, &n_channels, STBI_rgb_alpha);
		unit = sizeof(float);
	} else {
		pixels = stbi_load(file_path.c_str(), &width, &height, &n_channels, STBI_rgb_alpha);
		unit = sizeof(u8);
	}

	TextureLoadData *load_data = ctx.arena.push<TextureLoadData>();
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

static Asset *texture_asset_allocate(
	const AssetLoadContext &ctx, const AssetLoadResult &result,
	gfx::Device &device
)
{
	TextureLoadData *load_data = (TextureLoadData *)result.data;
	
	VkFormat format = load_data->is_hdr
		? VK_FORMAT_R32G32B32A32_SFLOAT
		: VK_FORMAT_R8G8B8A8_UNORM;

	gfx::Texture *texture = device.alloc_texture_2d(load_data->width, load_data->height, format, 5);

	return ctx.arena.push<TextureAsset>(texture, device);
}

static void texture_upload(
	Asset *asset,
	const AssetLoadContext &ctx, const AssetLoadResult &result,
	gfx::CommandBuffer &cmd,
	gfx::GpuBuffer *stage, u64 stage_base
)
{
	TextureLoadData *load_data = (TextureLoadData *)result.data;
	TextureAsset *texture_asset = asset->as<TextureAsset>();

	stage->write(load_data->pixels, result.stage_size, stage_base);

	VkImageMemoryBarrier2 copy_barrier = gfx::sync::texture_memory_barrier(
		texture_asset->texture,
		{ VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE },
		{ VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT },
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		0, VK_REMAINING_MIP_LEVELS,
		0, VK_REMAINING_ARRAY_LAYERS
	);

	cmd.pipeline_barrier(0, {}, {}, { copy_barrier });
	
	cmd.copy_buffer_to_texture(stage, texture_asset->texture, stage_base);

	VkImageMemoryBarrier2 blit_barrier = gfx::sync::texture_memory_barrier(
		texture_asset->texture,
		{ VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT },
		{ VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT },
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		0, VK_REMAINING_MIP_LEVELS,
		0, VK_REMAINING_ARRAY_LAYERS
	);

	cmd.pipeline_barrier(0, {}, {}, { blit_barrier });
	cmd.generate_mipmaps(texture_asset->texture);

	stbi_image_free(load_data->pixels);
}

AssetSerializer ast::get_texture_serializer()
{
	AssetSerializer texture_serializer = {};
	texture_serializer.load = texture_load;
	texture_serializer.allocate = texture_asset_allocate;
	texture_serializer.upload = texture_upload;

	return texture_serializer;
}
