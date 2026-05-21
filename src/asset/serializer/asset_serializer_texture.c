
typedef struct AST_TextureLoadData AST_TextureLoadData;
struct AST_TextureLoadData
{
	u32 width;
	u32 height;
	b32 is_hdr;
	void *pixel_data;
};

internal AST_SerializerPipelineData
AST_TextureSerializerCpu(const AST_Context *ctx, Arena *load_scope)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	String8 file_path = AST_ContextSystemFilePath(ctx, scratch.arena);

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

	AST_TextureLoadData *tex_load_data = ArenaPushArray(load_scope, AST_TextureLoadData, 1);
	tex_load_data->width = w;
	tex_load_data->height = h;
	tex_load_data->is_hdr = is_hdr;
	tex_load_data->pixel_data = px;

	AST_SerializerPipelineData result = {0};
	result.data = tex_load_data;
	result.stage_size = w * h * stride * 4; // RGBA = 4 values ppx.
	result.failed = px == NULL;
	result.dependency_count = 0;
	result.watch_path_count = 0;

	ScratchRelease(&scratch);

	return result;
}

internal void
AST_TextureSerializerAlloc(const AST_Context *ctx,
						   AST_SerializerPipelineData *data,
						   AST_Asset *out,
						   Arena *arena)
{
	GFX_Device *device = ctx->assets->device;
	
	AST_TextureLoadData *tex_data = data->data;

	VkFormat format = tex_data->is_hdr
		? VK_FORMAT_R32G32B32A32_SFLOAT
		: VK_FORMAT_R8G8B8A8_UNORM;

	out->texture_data.key = GFX_DeviceTextureAlloc2D(device, tex_data->width, tex_data->height, format, 5);
}

internal void
AST_TextureSerializerReload(const AST_Context *ctx,
							AST_SerializerPipelineData *data,
							AST_Asset *existing)
{
	DebugLogW(ctx->log_channel, "Reloading not implemented yet.");
}

internal void
AST_TextureSerializerGpu(const AST_Context *ctx,
						 AST_SerializerPipelineData *data,
						 AST_Asset *asset,
						 GFX_CmdBuffer *cmd,
						 GFX_BufferKey stage, u64 stage_base)
{
	GFX_Device *device = ctx->assets->device;
	
	AST_TextureLoadData *tex_data = data->data;
	GFX_Texture *gfx_texture = GFX_DeviceTextureFromKey(device, asset->texture_data.key);

	GFX_DeviceBufferWrite(device, stage, tex_data->pixel_data, data->stage_size, stage_base);

	GFX_AccessSt copy_src = { VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE };
	GFX_AccessSt copy_dst = { VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };
	GFX_AccessSt blit_dst = { VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };

	VkImageMemoryBarrier2 copy_barrier = GFX_SyncTextureBarrier(gfx_texture,
																&copy_src,
																&copy_dst,
																VK_IMAGE_LAYOUT_UNDEFINED,
																VK_IMAGE_LAYOUT_GENERAL,
																0, VK_REMAINING_MIP_LEVELS,
																0, VK_REMAINING_ARRAY_LAYERS);

	GFX_CmdPipelineBarrier(cmd, 0, 0, NULL, 0, NULL, 1, &copy_barrier);
	
	GFX_CmdCopyBufferToTextureWhole(cmd, stage, asset->texture_data.key, stage_base);

	VkImageMemoryBarrier2 blit_barrier = GFX_SyncTextureBarrier(gfx_texture,
																&copy_dst,
																&blit_dst,
																VK_IMAGE_LAYOUT_GENERAL,
																VK_IMAGE_LAYOUT_GENERAL,
																0, VK_REMAINING_MIP_LEVELS,
																0, VK_REMAINING_ARRAY_LAYERS);

	GFX_CmdPipelineBarrier(cmd, 0, 0, NULL, 0, NULL, 1, &blit_barrier);
	GFX_CmdGenerateMipmaps(cmd, asset->texture_data.key);
}

internal void
AST_TextureSerializerEnd(AST_SerializerPipelineData *data)
{
	AST_TextureLoadData *tex_data = data->data;

	stbi_image_free(tex_data->pixel_data);
}

internal void
AST_TextureSerializerDispose(AST_Asset *asset, AST_Assets *assets)
{
	GFX_DeviceTextureDestroy(assets->device, asset->texture_data.key);
}

internal AST_Serializer
AST_GetTextureSerializer(void)
{
	static AST_Serializer texture_serializer = {
		.Cpu     = AST_TextureSerializerCpu,
		.Alloc   = AST_TextureSerializerAlloc,
		.Reload  = AST_TextureSerializerReload,
		.Gpu     = AST_TextureSerializerGpu,
		.End     = AST_TextureSerializerEnd,
		.Dispose = AST_TextureSerializerDispose,
	};

	return texture_serializer;
}
