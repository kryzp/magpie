
static const A_Skeleton *A_ModelDataGetSkeletonByName(const A_ModelData *asset, String8 name)
{
	for (u32 i = 0; i < asset->skeleton_count; i++)
	{
		if (String8Match(asset->skeletons[i].name, name))
			return &asset->skeletons[i];
	}

	return NULL;
}

static const A_AnimClip *A_ModelDataGetAnimClipByName(const A_ModelData *asset, String8 name)
{
	for (u32 i = 0; i < asset->clip_count; i++)
	{
		if (String8Match(asset->clips[i].name, name))
			return &asset->clips[i];
	}

	return NULL;
}
