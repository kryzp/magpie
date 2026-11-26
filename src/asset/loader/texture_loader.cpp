#include "asset/asset_loader.h"
#include "asset/asset_system.h"
#include "graphics/device.h"
#include "graphics/sync.h"

#include <stb/stb_image.h>
#include <ktx.h>

struct TextureLoadData : ast::LoadData {
	unsigned width = 0;
	unsigned height = 0;
	bool is_hdr = false;
	void *pixel_data = nullptr;

	bool flip_x = false;
	bool flip_y = false;
	bool flip_z = false;

	ktxTexture2 *ktx = nullptr;
	VkFormat ktx_format = VK_FORMAT_UNDEFINED;
	unsigned ktx_levels = 1;
	bool ktx_generate_mips = false;

	void Release()
	{
		if (pixel_data) {
			stbi_image_free(pixel_data);
			pixel_data = nullptr;
		}

		if (ktx) {
			ktxTexture_Destroy(ktxTexture(ktx));
			ktx = nullptr;
		}
	}
};

const Vector<String> &ast::TextureLoader::GetFileExtensions() const
{
	static const Vector<String> file_extensions = {
		"png",
		"jpg",
		"hdr",
		"ktx2"
	};

	return file_extensions;
}

ast::LoadResult ast::TextureLoader::Load(const LoaderContext &ctx)
{
	gfx::Device *device = gfx::Device::Get();

	ast::LoadResult result;
	result.failed = true;

	String file_path = AssetSystem::Get()->GetSystemFilePath(ctx.metadata.path);

	// todo: HasExtension utility in load context.
	if (file_path.size() > 5 && file_path.compare(file_path.size() - 5, 5, ".ktx2") == 0) {

		ktxTexture2 *ktx = nullptr;

		KTX_error_code err = ktxTexture2_CreateFromNamedFile(file_path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx);
		if (err != KTX_SUCCESS) {
			DebugLogE(ctx.log_channel, "Failed to open KTX2 (%s): %s", file_path.c_str(), ktxErrorString(err));
			return result;
		}

		TextureLoadData *d = new TextureLoadData();
		d->ktx = ktx;
		result.user_data = d;

		if (ktx->numDimensions != 2 || ktx->numLayers > 1 || ktx->numFaces > 1 || ktx->baseDepth > 1)
			return result;

		d->flip_x = ktx->orientation.x == KTX_ORIENT_X_LEFT;
		d->flip_y = ktx->orientation.y == KTX_ORIENT_Y_UP;
		d->flip_z = ktx->orientation.z == KTX_ORIENT_Z_IN;

		// basis ETC1S / UASTC
		if (ktxTexture2_NeedsTranscoding(ktx)) {

			ktx_transcode_fmt_e target;

			if (device->SupportsTextureCompressionBC())
				target = KTX_TTF_BC7_RGBA;
			else if (device->SupportsTextureCompressionASTC())
				target = KTX_TTF_ASTC_4x4_RGBA;
			else
				target = KTX_TTF_RGBA32;

			if (ktxTexture2_TranscodeBasis(ktx, target, 0) != KTX_SUCCESS)
				return result;
		}

		VkFormat format = static_cast<VkFormat>(ktx->vkFormat);
		if (format == VK_FORMAT_UNDEFINED || !device->FormatSupports(format, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
			return result;

		bool can_blit = !gfx::TextureFormatIsBlockCompressed(format) && device->FormatSupports(format, VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

		d->width = ktx->baseWidth;
		d->height = ktx->baseHeight;
		d->ktx_format = format;
		d->ktx_levels = ktx->numLevels;
		d->ktx_generate_mips = can_blit && (ktx->numLevels == 1 || ktx->generateMipmaps);

		result.stage_size = ktxTexture_GetDataSize(ktxTexture(ktx));
		result.failed = false;
	} else {
		int w = 0, h = 0, n = 0;
		void *px = nullptr;
		u64 stride = 0;

		bool is_hdr = stbi_is_hdr(file_path.c_str());

		if (is_hdr) {
			px = stbi_loadf(file_path.c_str(), &w, &h, &n, STBI_rgb_alpha);
			stride = sizeof(float);
		} else {
			px = stbi_load(file_path.c_str(), &w, &h, &n, STBI_rgb_alpha);
			stride = sizeof(u8);
		}

		TextureLoadData *tex_load_data = new TextureLoadData();
		tex_load_data->width = w;
		tex_load_data->height = h;
		tex_load_data->is_hdr = is_hdr;
		tex_load_data->pixel_data = px;

		result.user_data = tex_load_data;
		result.stage_size = w * h * stride * 4; // RGBA = 4 values ppx.
		result.failed = px == nullptr;
	}

	return result;
}

static VkFormat FormatWithSRGB(VkFormat format, bool srgb)
{
	struct Pair { VkFormat unorm, srgb; };

	static const Pair pairs[] = {
		{ VK_FORMAT_R8G8B8A8_UNORM,            VK_FORMAT_R8G8B8A8_SRGB },
		{ VK_FORMAT_B8G8R8A8_UNORM,            VK_FORMAT_B8G8R8A8_SRGB },
		{ VK_FORMAT_R8G8B8_UNORM,              VK_FORMAT_R8G8B8_SRGB },
		{ VK_FORMAT_R8G8_UNORM,                VK_FORMAT_R8G8_SRGB },
		{ VK_FORMAT_R8_UNORM,                  VK_FORMAT_R8_SRGB },
		{ VK_FORMAT_BC1_RGB_UNORM_BLOCK,       VK_FORMAT_BC1_RGB_SRGB_BLOCK },
		{ VK_FORMAT_BC1_RGBA_UNORM_BLOCK,      VK_FORMAT_BC1_RGBA_SRGB_BLOCK },
		{ VK_FORMAT_BC2_UNORM_BLOCK,           VK_FORMAT_BC2_SRGB_BLOCK },
		{ VK_FORMAT_BC3_UNORM_BLOCK,           VK_FORMAT_BC3_SRGB_BLOCK },
		{ VK_FORMAT_BC7_UNORM_BLOCK,           VK_FORMAT_BC7_SRGB_BLOCK },
		{ VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,   VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK },
		{ VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK },
		{ VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK },
		{ VK_FORMAT_ASTC_4x4_UNORM_BLOCK,      VK_FORMAT_ASTC_4x4_SRGB_BLOCK },
		{ VK_FORMAT_ASTC_5x5_UNORM_BLOCK,      VK_FORMAT_ASTC_5x5_SRGB_BLOCK },
		{ VK_FORMAT_ASTC_6x6_UNORM_BLOCK,      VK_FORMAT_ASTC_6x6_SRGB_BLOCK },
		{ VK_FORMAT_ASTC_8x8_UNORM_BLOCK,      VK_FORMAT_ASTC_8x8_SRGB_BLOCK },
	};

	for (const Pair &p : pairs) {
		if (format == p.unorm || format == p.srgb)
			return srgb ? p.srgb : p.unorm;
	}

	return format;
}

void ast::TextureLoader::Alloc(const LoaderContext &ctx, LoadResult *result, Asset *asset)
{
	gfx::Device *device = gfx::Device::Get();
	TextureLoadData *tex_data = static_cast<TextureLoadData *>(result->user_data);

	VkFormat format = VK_FORMAT_UNDEFINED;
	int mips = 1;

	if (tex_data->ktx) {
		format = FormatWithSRGB(tex_data->ktx_format, ctx.metadata.srgb);
		mips = tex_data->ktx_generate_mips ? 32u : tex_data->ktx_levels; // TextureAlloc clamps so just pick the highest mip count we can
	} else {
		format = FormatWithSRGB(VK_FORMAT_R8G8B8A8_UNORM, ctx.metadata.srgb);
		if (tex_data->is_hdr)
			format = VK_FORMAT_R32G32B32A32_SFLOAT;
		mips = 5;
	}

	asset->texture.key = device->TextureAlloc2D(tex_data->width, tex_data->height, format, mips);

	if (vkSetDebugUtilsObjectNameEXT) {
		VkDebugUtilsObjectNameInfoEXT name = {};
		name.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		name.objectType = VK_OBJECT_TYPE_IMAGE;
		name.objectHandle = reinterpret_cast<u64>(device->TextureFromKey(asset->texture.key)->vk_handle);
		name.pObjectName = ctx.metadata.path.c_str();

		vkSetDebugUtilsObjectNameEXT(device->vk_device, &name);
	}
}

void ast::TextureLoader::UploadGPU(
	const LoaderContext &ctx,
	LoadResult *result,
	Asset *asset,
	const gfx::CmdBuffer &cmd,
	gfx::ResourceKey stage,
	u64 stage_offset
)
{
	gfx::Device *device = gfx::Device::Get();
	TextureLoadData *tex_data = static_cast<TextureLoadData *>(result->user_data);
	gfx::Texture *gfx_texture = device->TextureFromKey(asset->texture.key);

	gfx::AccessState copy_src = {
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		VK_ACCESS_2_NONE
	};

	gfx::AccessState copy_dst = {
		VK_PIPELINE_STAGE_2_COPY_BIT,
		VK_ACCESS_2_TRANSFER_WRITE_BIT
	};

	gfx::AccessState blit_dst = {
		VK_PIPELINE_STAGE_2_BLIT_BIT,
		VK_ACCESS_2_TRANSFER_WRITE_BIT
	};

	gfx::AccessState top_of_pipe = {
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		VK_ACCESS_2_NONE
	};

	gfx::AccessState shader_read = {
		VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		VK_ACCESS_2_SHADER_SAMPLED_READ_BIT
	};

	if (tex_data->ktx) {
		ktxTexture *k = ktxTexture(tex_data->ktx);
		device->BufferWrite(stage, ktxTexture_GetData(k), result->stage_size, stage_offset);

		VkImageMemoryBarrier2 to_copy = gfx::sync::TextureBarrier(
			gfx_texture,
			top_of_pipe,
			copy_dst,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
			0, VK_REMAINING_MIP_LEVELS,
			0, VK_REMAINING_ARRAY_LAYERS
		);

		cmd.PipelineBarrier(0, 0, nullptr, 0, nullptr, 1, &to_copy);

		unsigned levels = tex_data->ktx_generate_mips ? 1 : tex_data->ktx_levels;

		if (levels > gfx_texture->mipmap_count)
			levels = gfx_texture->mipmap_count;

		Vector<gfx::BufferImageCopy> regions(levels);

		for (u32 l = 0; l < levels; l++) {
			ktx_size_t off = 0;
			ktxTexture_GetImageOffset(k, l, 0, 0, &off);
			gfx::BufferImageCopy &r = regions[l];
			r.buffer_offset = stage_offset + off;
			r.image_subresource = { VK_IMAGE_ASPECT_COLOR_BIT, l, 0, 1 };
			r.width  = (tex_data->width  >> l) ? (tex_data->width  >> l) : 1;
			r.height = (tex_data->height >> l) ? (tex_data->height >> l) : 1;
			r.depth = 1;
		}

		cmd.CopyBufferToTexture(stage, asset->texture.key, levels, regions.data());

		VkImageMemoryBarrier2 after_copy = gfx::sync::TextureBarrier(
			gfx_texture,
			copy_dst,
			tex_data->ktx_generate_mips ? blit_dst : shader_read,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
			0, VK_REMAINING_MIP_LEVELS,
			0, VK_REMAINING_ARRAY_LAYERS
		);

		cmd.PipelineBarrier(0, 0, nullptr, 0, nullptr, 1, &after_copy);

		if (tex_data->ktx_generate_mips)
			cmd.GenerateMipmaps(asset->texture.key);

	} else {
		device->BufferWrite(stage, tex_data->pixel_data, result->stage_size, stage_offset);

		VkImageMemoryBarrier2 copy_barrier = gfx::sync::TextureBarrier(
			gfx_texture,
			copy_src,
			copy_dst,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			0, VK_REMAINING_MIP_LEVELS,
			0, VK_REMAINING_ARRAY_LAYERS
		);

		cmd.PipelineBarrier(0, 0, nullptr, 0, nullptr, 1, &copy_barrier);

		cmd.CopyBufferToTextureWhole(stage, asset->texture.key, stage_offset);

		VkImageMemoryBarrier2 blit_barrier = gfx::sync::TextureBarrier(
			gfx_texture,
			copy_dst,
			blit_dst,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_GENERAL,
			0, VK_REMAINING_MIP_LEVELS,
			0, VK_REMAINING_ARRAY_LAYERS
		);

		cmd.PipelineBarrier(0, 0, nullptr, 0, nullptr, 1, &blit_barrier);

		cmd.GenerateMipmaps(asset->texture.key);
	}
}

void ast::TextureLoader::DestroyIntermediateResources(LoadResult *result)
{
	TextureLoadData *tex_data = static_cast<TextureLoadData *>(result->user_data);

	if (tex_data)
		tex_data->Release();
}

void ast::TextureLoader::DestroyAsset(Asset *asset)
{
	gfx::Device::Get()->TextureDestroy(asset->texture.key);
}

ast::ResidencyLevel ast::TextureLoader::CalculateResidency(const LoadResidencyCtx &ctx)
{
	ResidencyLevel level;
	level.lod = 0;
	return level;
}
