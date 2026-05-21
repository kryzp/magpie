#ifndef ASSET_DEF_H
#define ASSET_DEF_H

typedef struct AST_AssetTexture AST_AssetTexture;
struct AST_AssetTexture
{
	GFX_TextureKey key;
};

typedef struct AST_AssetShader AST_AssetShader;
struct AST_AssetShader
{
	GFX_ShaderKey key;
};

typedef struct AST_AssetSound AST_AssetSound;
struct AST_AssetSound
{
	AUD_BufferHandle buffer;
};

typedef struct AST_AssetModel AST_AssetModel;
struct AST_AssetModel
{
	u32 sub_model_count;
	AST_SubModel *sub_models;

	u32 skeleton_count;
	AST_Skeleton *skeletons;
			
	u32 clip_count;
	AST_AnimClip *clips;
};

internal const AST_SubModel *AST_AssetModelGetSubModel(const AST_AssetModel *asset, u32 index);
internal const AST_Skeleton *AST_AssetModelGetSkeleton(const AST_AssetModel *asset, String8 name);
internal const AST_AnimClip *AST_AssetModelGetAnimClip(const AST_AssetModel *asset, String8 name);

typedef struct AST_AssetScript AST_AssetScript;
struct AST_AssetScript
{
	SCR_ScriptRef ref;
};

typedef struct AST_Asset AST_Asset;
struct AST_Asset
{
	AST_Type type;
	AST_Handle handle;

	union
	{
		AST_AssetTexture texture_data;
		AST_AssetShader  shader_data;
		AST_AssetSound   sound_data;
		AST_AssetModel   model_data;
		AST_AssetScript  script_data;
	};
};

#endif // ASSET_DEF_H
