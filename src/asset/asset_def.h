#ifndef ASSET_DEF_H
#define ASSET_DEF_H

typedef struct A_TextureData A_TextureData;
struct A_TextureData
{
	G_TextureKey key;
};

typedef struct A_ShaderData A_ShaderData;
struct A_ShaderData
{
	G_ShaderKey key;
};

typedef struct A_SoundData A_SoundData;
struct A_SoundData
{
	AU_BufferHandle buffer;
};

typedef struct A_ModelData A_ModelData;
struct A_ModelData
{
	u32 sub_model_count;
	A_SubModel *sub_models;

	u32 skeleton_count;
	A_Skeleton *skeletons;
			
	u32 clip_count;
	A_AnimClip *clips;
};

internal const A_SubModel *A_ModelDataGetSubModel(const A_ModelData *asset, u32 index);
internal const A_Skeleton *A_ModelDataGetSkeleton(const A_ModelData *asset, String8 name);
internal const A_AnimClip *A_ModelDataGetAnimClip(const A_ModelData *asset, String8 name);

typedef struct A_ScriptData A_ScriptData;
struct A_ScriptData
{
	S_Ref ref;
};

typedef struct A_Asset A_Asset;
struct A_Asset
{
	A_Handle handle;

	union
	{
		A_TextureData texture;
		A_ShaderData  shader;
		A_SoundData   sound;
		A_ModelData   model;
		A_ScriptData  script;
	};
};

#endif // ASSET_DEF_H
