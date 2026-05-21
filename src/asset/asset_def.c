
internal const AST_SubModel *
AST_AssetModelGetSubModel(const AST_AssetModel *asset, u32 index)
{
	if (index >= asset->sub_model_count)
		return NULL;
	
	return &asset->sub_models[index];
}

internal const AST_Skeleton *
AST_AssetModelGetSkeleton(const AST_AssetModel *asset, String8 name)
{
	for (u32 i = 0; i < asset->skeleton_count; i++)
	{
		if (String8Match(asset->skeletons[i].name, name))
			return &asset->skeletons[i];
	}

	return NULL;
}

internal const AST_AnimClip *
AST_AssetModelGetAnimClip(const AST_AssetModel *asset, String8 name)
{
	for (u32 i = 0; i < asset->clip_count; i++)
	{
		if (String8Match(asset->clips[i].name, name))
			return &asset->clips[i];
	}

	return NULL;
}
