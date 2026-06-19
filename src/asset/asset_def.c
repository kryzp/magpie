
internal const A_SubModel *
A_ModelDataGetSubModel(const A_ModelData *asset, u32 index)
{
	if (index >= asset->sub_model_count)
		return NULL;
	
	return &asset->sub_models[index];
}

internal const A_Skeleton *
A_ModelDataGetSkeleton(const A_ModelData *asset, String8 name)
{
	for (u32 i = 0; i < asset->skeleton_count; i++)
	{
		if (String8Match(asset->skeletons[i].name, name))
			return &asset->skeletons[i];
	}

	return NULL;
}

internal const A_AnimClip *
A_ModelDataGetAnimClip(const A_ModelData *asset, String8 name)
{
	for (u32 i = 0; i < asset->clip_count; i++)
	{
		if (String8Match(asset->clips[i].name, name))
			return &asset->clips[i];
	}

	return NULL;
}
