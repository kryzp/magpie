#ifndef ASSET_STRUCT_H
#define ASSET_STRUCT_H

typedef struct A_TextureAsset A_TextureAsset;
struct A_TextureAsset
{
	G_ResourceKey key;
	u8 loaded_mip_count;
	u8 max_mip_count;
};

typedef struct A_ShaderAsset A_ShaderAsset;
struct A_ShaderAsset
{
	G_ResourceKey key;
};

typedef struct A_SoundAsset A_SoundAsset;
struct A_SoundAsset
{
	AU_BufferHandle buffer;
};

typedef struct A_ModelAsset A_ModelAsset;
struct A_ModelAsset
{
	u32 sub_model_count;
	A_SubModel *sub_models;

	u32 skeleton_count;
	A_Skeleton *skeletons;
			
	u32 clip_count;
	A_AnimClip *clips;
};

typedef struct A_ScriptAsset A_ScriptAsset;
struct A_ScriptAsset
{
	S_Ref ref;
};

typedef union A_Asset A_Asset;
union A_Asset
{
	A_TextureAsset texture;
	A_ShaderAsset shader;
	A_SoundAsset sound;
	A_ModelAsset model;
	A_ScriptAsset script;
};

#endif // ASSET_STRUCT_H
