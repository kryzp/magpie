
internal GFX_TextureKey
AST_AssetTextureGet(const AST_Asset *asset)
{
	return asset->texture.key;
}

internal GFX_ShaderKey
AST_AssetShaderGet(const AST_Asset *asset)
{
	return asset->shader.key;
}

internal AUD_BufferHandle
AST_AssetSoundGetBuffer(const AST_Asset *asset)
{
	return asset->sound.buffer;
}

internal const AST_SubModel *
AST_AssetModelGetSubModel(const AST_Asset *asset, u32 index)
{
	if (index >= asset->model.sub_model_count)
		return NULL;
	
	return &asset->model.sub_models[index];
}

internal const AST_Skeleton *
AST_AssetModelGetSkeleton(const AST_Asset *asset, String8 name)
{
	for (u32 i = 0; i < asset->model.skeleton_count; i++)
	{
		if (String8Match(asset->model.skeletons[i].name, name))
			return &asset->model.skeletons[i];
	}

	return NULL;
}

internal const AST_AnimClip *
AST_AssetModelGetAnimClip(const AST_Asset *asset, String8 name)
{
	for (u32 i = 0; i < asset->model.clip_count; i++)
	{
		if (String8Match(asset->model.clips[i].name, name))
			return &asset->model.clips[i];
	}

	return NULL;
}
