
typedef struct A_TextureLoadData A_TextureLoadData;
struct A_TextureLoadData
{
	u32 width;
	u32 height;
	b32 is_hdr;
	void *pixel_data;
};

internal A_LoadResult A_TextureLoaderLoad(const A_LCTX *ctx,
										  Arena *result_arena)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	String8 file_path = A_GetSystemFilePath(scratch.arena, ctx->metadata.path);

	i32 w, h, n;
	void *px;
	u64 stride;

	b32 is_hdr = stbi_is_hdr((const char *)file_path.str);

	if (is_hdr)
	{
		px = stbi_loadf((const char *)file_path.str, &w, &h, &n, STBI_rgb_alpha);
		stride = sizeof(f32);
	}
	else
	{
		px = stbi_load((const char *)file_path.str, &w, &h, &n, STBI_rgb_alpha);
		stride = sizeof(u8);
	}

	A_TextureLoadData *tex_load_data = ArenaPushArray(result_arena, A_TextureLoadData, 1);
	tex_load_data->width = w;
	tex_load_data->height = h;
	tex_load_data->is_hdr = is_hdr;
	tex_load_data->pixel_data = px;

	A_LoadResult result = {0};
	result.user_data = tex_load_data;
	result.stage_size = w * h * stride * 4; // RGBA = 4 values ppx.
	result.failed = px == NULL;
	
	ScratchRelease(&scratch);

	return result;
}

internal void A_TextureLoaderAlloc(const A_LCTX *ctx,
								   A_LoadResult *result,
								   Arena *asset_arena,
								   A_Asset *asset)
{
	A_TextureLoadData *tex_data = result->user_data;
	
	VkFormat format = tex_data->is_hdr
		? VK_FORMAT_R32G32B32A32_SFLOAT
		: VK_FORMAT_R8G8B8A8_UNORM;

	asset->texture.key = G_TextureAlloc2D(tex_data->width, tex_data->height, format, 5);
}

internal void A_TextureLoaderUploadGPU(const A_LCTX *ctx,
									   A_LoadResult *result,
									   A_Asset *asset,
									   G_CmdBuffer *cmd,
									   G_ResourceKey stage,
									   u64 stage_offset)
{
	A_TextureLoadData *tex_data = result->user_data;
	
	G_Texture *gfx_texture = G_TextureFromKey(asset->texture.key);
	
	G_BufferWrite(stage, tex_data->pixel_data, result->stage_size, stage_offset);

	G_AccessSt copy_src = { VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE };
	G_AccessSt copy_dst = { VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };
	G_AccessSt blit_dst = { VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };

	VkImageMemoryBarrier2 copy_barrier = G_SyncTextureBarrier(gfx_texture,
															  &copy_src,
															  &copy_dst,
															  VK_IMAGE_LAYOUT_UNDEFINED,
															  VK_IMAGE_LAYOUT_GENERAL,
															  0, VK_REMAINING_MIP_LEVELS,
															  0, VK_REMAINING_ARRAY_LAYERS);

	G_CmdPipelineBarrier(cmd, 0, 0, NULL, 0, NULL, 1, &copy_barrier);
	
	G_CmdCopyBufferToTextureWhole(cmd, stage, asset->texture.key, stage_offset);

	VkImageMemoryBarrier2 blit_barrier = G_SyncTextureBarrier(gfx_texture,
															  &copy_dst,
															  &blit_dst,
															  VK_IMAGE_LAYOUT_GENERAL,
															  VK_IMAGE_LAYOUT_GENERAL,
															  0, VK_REMAINING_MIP_LEVELS,
															  0, VK_REMAINING_ARRAY_LAYERS);

	G_CmdPipelineBarrier(cmd, 0, 0, NULL, 0, NULL, 1, &blit_barrier);
	G_CmdGenerateMipmaps(cmd, asset->texture.key);
}

internal void A_TextureLoaderDestroyIntermediateResources(A_LoadResult *result)
{
	A_TextureLoadData *tex_data = result->user_data;

	stbi_image_free(tex_data->pixel_data);
}

internal void A_TextureLoaderDestroyAsset(A_Asset *asset)
{
	G_TextureDestroy(asset->texture.key);
}

internal A_LoaderAPI A_GetTextureLoaderAPI(void)
{
	static A_LoaderAPI texture_loader_api = {
		.Load = A_TextureLoaderLoad,
		.Alloc = A_TextureLoaderAlloc,
		.UploadGPU = A_TextureLoaderUploadGPU,
		.DestroyIntermediateResources = A_TextureLoaderDestroyIntermediateResources,
		.DestroyAsset = A_TextureLoaderDestroyAsset
	};

	return texture_loader_api;
}
