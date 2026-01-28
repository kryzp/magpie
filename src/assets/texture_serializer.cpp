#include "texture_serializer.h"

#include "graphics/sync.h"

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ext/stb_image_write.h"

using namespace ast;

static void serialize(AssetManager &assets, const AssetMetaData &metadata, const AssetHandle &handle, const FileStream &fs)
{
}

static Asset *try_load_data(AssetManager &assets, const AssetMetaData &metadata)
{
	const String &file_path = assets.get_system_file_path(metadata.file_path);

	bool failed_to_load = false;

	int width = 0, height = 0, n_channels = 0;
	void *pixels = nullptr;

	VkFormat format = (VkFormat)0;
	u64 unit = 0;

	if (stbi_is_hdr(file_path.c_str())) {
		pixels = stbi_loadf(file_path.c_str(), &width, &height, &n_channels, STBI_rgb_alpha);
		format = VK_FORMAT_R32G32B32A32_SFLOAT;
		unit = sizeof(float);
	} else {
		pixels = stbi_load(file_path.c_str(), &width, &height, &n_channels, STBI_rgb_alpha);
		format = VK_FORMAT_R8G8B8A8_UNORM;
		unit = sizeof(u8);
	}

	gfx::Texture *gfx_texture = nullptr;
	gfx::Device &device = assets.get_device();

	if (pixels) {
		gfx_texture = device.alloc_texture_2d(width, height, format, 4);

		u64 memory_size = width * height * 4 * unit;

		gfx::GpuBuffer *staging_buffer = device.alloc_buffer(
			VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			memory_size
		);

		staging_buffer->write(pixels, memory_size, 0);

		gfx::CommandBuffer cmd = device.begin_submit();

		auto copy_barrier = gfx::sync::texture_memory_barrier(
			gfx_texture,
			gfx::sync::get_src_texture_access(gfx::TEXTURE_ACCESS_UNDEFINED),
			gfx::sync::get_dst_texture_access(gfx::TEXTURE_ACCESS_COPY_DST),
			0, gfx_texture->get_mipmap_count(),
			0, gfx_texture->get_layer_count()
		);

		cmd.pipeline_barrier(0, {}, {}, { copy_barrier });
		cmd.copy_buffer_to_texture(staging_buffer, gfx_texture);

		auto blit_barrier = gfx::sync::texture_memory_barrier(
			gfx_texture,
			gfx::sync::get_src_texture_access(gfx::TEXTURE_ACCESS_COPY_DST),
			gfx::sync::get_dst_texture_access(gfx::TEXTURE_ACCESS_BLIT_DST),
			0, gfx_texture->get_mipmap_count(),
			0, gfx_texture->get_layer_count()
		);

		cmd.pipeline_barrier(0, {}, {}, { blit_barrier });
		cmd.generate_mipmaps(gfx_texture);

		device.end_submit(cmd);

		device.wait_idle();
		device.destroy_buffer(staging_buffer);
	} else {
		failed_to_load = true;
	}

	TextureAsset *asset = new TextureAsset(gfx_texture, device);

	if (failed_to_load)
		asset->set_flag(ASSET_FLAG_INVALID, true);

	return asset;
}

AssetSerializer ast::get_texture_serializer()
{
	AssetSerializer texture_serializer = {};
	texture_serializer.serialize = serialize;
	texture_serializer.try_load_data = try_load_data;

	return texture_serializer;
}
