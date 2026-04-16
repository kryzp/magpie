#ifndef ASSET_ASSET_H
#define ASSET_ASSET_H

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
#define AssetDef(name) if (String8Match(str, Str8(#name))) return AST_Type_##name;
#include "asset_definitions.inc"
#undef AssetDef
	
	AssertTrue(false && "Unknown Asset Name.");

	return AST_Type_COUNT;
}

internal inline String8
AST_StringFromType(Arena *arena, AST_Type type)
{
#define AssetDef(name) if (type == AST_Type_##name) return Str8(#name);
#include "asset_definitions.inc"
#undef AssetDef
	
	AssertTrue(false && "Unknown Asset Type.");

	return (String8) {0};
}

typedef enum AST_State
{
	AST_State_Unloaded,
	AST_State_CpuStage,
	AST_State_WaitingForDependencies,
	AST_State_GpuStage,
	AST_State_Ready,
	AST_State_Failed,
	AST_State_COUNT
}
AST_State;

internal inline b32
AST_StateIsLoading(AST_State st)
{
	return
		st == AST_State_CpuStage ||
		st == AST_State_WaitingForDependencies ||
		st == AST_State_GpuStage;
}

internal inline b32
AST_StateNeedsLoad(AST_State st)
{
	return
		st == AST_State_Unloaded ||
		st == AST_State_Failed;
}

internal inline b32
AST_StateIsLoaded(AST_State st)
{
	return
		st == AST_State_Ready;
}

internal inline b32
AST_StateIsFinalized(AST_State st)
{
	return
		st == AST_State_Ready ||
		st == AST_State_Failed;
}

typedef struct AST_MetaData AST_MetaData;
struct AST_MetaData
{
	String8 path;
};

typedef struct AST_Handle AST_Handle;
struct AST_Handle
{
	u32 index;
	u32 generation;
};

internal inline AST_Handle
AST_HandleNull(void)
{
	AST_Handle handle = {0};
	handle.index = -1u;
	handle.generation = 0;

	return handle;
}

internal inline b32
AST_HandleIsNull(AST_Handle handle)
{
	return (handle.index == 0 &&
			handle.generation == 0);
}

internal inline b32
AST_HandleMatch(AST_Handle a, AST_Handle b)
{
	return (a.index == b.index &&
			a.generation == b.generation);
}

typedef struct AST_Asset AST_Asset;
struct AST_Asset
{
	AST_Type type;
	AST_Handle handle;

	/*
	union
	{
		struct
		{
		}
		texture;

		struct
		{
		}
		shader;

		struct
		{
		}
		model;

		struct
		{
		}
		sound;
	};
	*/
};

#endif // ASSET_ASSET_H
