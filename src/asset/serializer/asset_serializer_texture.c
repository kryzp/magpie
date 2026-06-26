
typedef struct A_TextureLoadData A_TextureLoadData;
struct A_TextureLoadData
{
	u32 width;
	u32 height;
	b32 is_hdr;
	void *pixel_data;
};

static A_SerializerPipelineData A_TextureSerializerCpu(const A_Context *ctx, Arena *load_scope)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	String8 file_path = A_ContextSystemFilePath(ctx, scratch.arena);

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

	A_TextureLoadData *tex_load_data = ArenaPushArray(load_scope, A_TextureLoadData, 1);
	tex_load_data->width = w;
	tex_load_data->height = h;
	tex_load_data->is_hdr = is_hdr;
	tex_load_data->pixel_data = px;

	A_SerializerPipelineData result = {0};
	result.data = tex_load_data;
	result.stage_size = w * h * stride * 4; // RGBA = 4 values ppx.
	result.failed = px == NULL;
	result.dependency_count = 0;
	result.watch_path_count = 0;

	ScratchRelease(&scratch);

	return result;
}

static void A_TextureSerializerAlloc(const A_Context *ctx,
						   A_SerializerPipelineData *data,
						   A_Asset *out,
						   Arena *arena)
{
	G_Device *device = ctx->assets->device;
	
	A_TextureLoadData *tex_data = data->data;

	VkFormat format = tex_data->is_hdr
		? VK_FORMAT_R32G32B32A32_SFLOAT
		: VK_FORMAT_R8G8B8A8_UNORM;

	out->texture.key = G_DeviceTextureAlloc2D(device, tex_data->width, tex_data->height, format, 5);
}

static void A_TextureSerializerReload(const A_Context *ctx,
							A_SerializerPipelineData *data,
							A_Asset *existing)
{
	DebugLogW(ctx->log_channel, "Reloading not implemented yet.");
}

static void A_TextureSerializerGpu(const A_Context *ctx,
						 A_SerializerPipelineData *data,
						 A_Asset *asset,
						 G_CmdBuffer *cmd,
						 G_BufferKey stage, u64 stage_base)
{
	G_Device *device = ctx->assets->device;
	
	A_TextureLoadData *tex_data = data->data;
	G_Texture *gfx_texture = G_DeviceTextureFromKey(device, asset->texture.key);

	G_DeviceBufferWrite(device, stage, tex_data->pixel_data, data->stage_size, stage_base);

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
	
	G_CmdCopyBufferToTextureWhole(cmd, stage, asset->texture.key, stage_base);

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

static void A_TextureSerializerEnd(A_SerializerPipelineData *data)
{
	A_TextureLoadData *tex_data = data->data;

	stbi_image_free(tex_data->pixel_data);
}

static void A_TextureSerializerDispose(A_Asset *asset, A_Assets *assets)
{
	G_DeviceTextureDestroy(assets->device, asset->texture.key);
}

static A_Serializer A_GetTextureSerializer(void)
{
	static A_Serializer texture_serializer = {
		.Cpu     = A_TextureSerializerCpu,
		.Alloc   = A_TextureSerializerAlloc,
		.Reload  = A_TextureSerializerReload,
		.Gpu     = A_TextureSerializerGpu,
		.End     = A_TextureSerializerEnd,
		.Dispose = A_TextureSerializerDispose,
	};

	return texture_serializer;
}
