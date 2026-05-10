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
#define AssetDef(name) AST_Type_##name,
#include "asset_definitions.inc"
#undef AssetDef
	AST_Type_COUNT
}
AST_Type;

internal inline AST_Type
AST_TypeFromString(String8 str)
{
#define AssetDef(name) if (String8Match(str, String8Lit(#name))) return AST_Type_##name;
#include "asset_definitions.inc"
#undef AssetDef
	
	AssertTrue(false && "Unknown Asset Name.");

	return AST_Type_COUNT;
}

internal inline String8
AST_StringFromType(Arena *arena, AST_Type type)
{
#define AssetDef(name) if (type == AST_Type_##name) return String8Lit(#name);
#include "asset_definitions.inc"
#undef AssetDef
	
	AssertTrue(false && "Unknown Asset Type.");

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

			AST_Skeleton skeleton;
			
			u32 clip_count;
			AST_AnimClip *clips;
		}
		model;
	};
};

#endif // ASSET_TYPE_H
