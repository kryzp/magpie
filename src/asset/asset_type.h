#ifndef ASSET_TYPE_H
#define ASSET_TYPE_H

typedef struct AST_MetaData AST_MetaData;
struct AST_MetaData
{
	String8 path;
};

typedef enum AST_Type
{
	AST_Type_Unknown,
#define AssetDef(name, upper) AST_Type_##name,
#include "asset_definitions.inc"
#undef AssetDef
	AST_Type_COUNT
}
AST_Type;

internal inline AST_Type
AST_TypeFromString(String8 str)
{
#define AssetDef(name, upper) if (String8Match(str, String8Lit(#name))) return AST_Type_##name;
#include "asset_definitions.inc"
#undef AssetDef
	
	DebugPrintB("Unknown Asset Name: %.*s",
				(i32)str.len,
				str.str);

	return AST_Type_COUNT;
}

internal inline String8
AST_StringFromType(Arena *arena, AST_Type type)
{
#define AssetDef(name, upper) if (type == AST_Type_##name) return String8Lit(#name);
#include "asset_definitions.inc"
#undef AssetDef
	
	DebugPrintB("Unknown Asset Type: %d", type);

	return (String8) {0};
}

typedef struct AST_Asset AST_Asset;
struct AST_Asset
{
	AST_Type type;
	AST_Handle handle;

	union
	{
		struct
		{
			GFX_TextureKey key;
		}
		texture;

		struct
		{
			GFX_ShaderKey key;
		}
		shader;

		struct
		{
			AUD_BufferHandle buffer;
		}
		sound;

		struct
		{
			u32 sub_model_count;
			AST_SubModel *sub_models;

			u32 skeleton_count;
			AST_Skeleton *skeletons;
			
			u32 clip_count;
			AST_AnimClip *clips;
		}
		model;
	};
};

// TODO: remove all this, instead replace union with

#if 0
union
{
	GFX_AssetTexture texture_data;
	GFX_AssetShader shader_data;
	GFX_AssetSound sound_data;
	// ...
}
#endif

internal GFX_TextureKey      AST_AssetTextureGet       (const AST_Asset *asset);
internal GFX_ShaderKey       AST_AssetShaderGet        (const AST_Asset *asset);
internal AUD_BufferHandle    AST_AssetSoundGetBuffer   (const AST_Asset *asset);
internal const AST_SubModel *AST_AssetModelGetSubModel (const AST_Asset *asset, u32 index);
internal const AST_Skeleton *AST_AssetModelGetSkeleton (const AST_Asset *asset, String8 name);
internal const AST_AnimClip *AST_AssetModelGetAnimClip (const AST_Asset *asset, String8 name);

#endif // ASSET_TYPE_H
