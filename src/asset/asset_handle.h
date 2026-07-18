#ifndef ASSET_HANDLE_H
#define ASSET_HANDLE_H

typedef enum A_Type
{
	A_Type_Null,
#define AssetDef(name, upper) A_Type_##name,
#include "asset_xmacro.inc"
#undef AssetDef
	A_Type_COUNT
}
A_Type;

static inline A_Type A_TypeFromString(String8 string)
{
#define AssetDef(name, upper) if (String8Match(string, String8Lit(#name))) return A_Type_##name;
#include "asset_xmacro.inc"
#undef AssetDef
	DebugPrintB("Unknown Asset Name: %.*s", String8VArg(string));
	return A_Type_Null;
}

static inline String8 A_StringFromType(Arena *arena, A_Type type)
{
#define AssetDef(name, upper) if (type == A_Type_##name) return String8Lit(#name);
#include "asset_xmacro.inc"
#undef AssetDef
	DebugPrintB("Unknown Asset Type: %d", type);
	return (String8){0};
}

typedef struct A_Handle A_Handle;
struct A_Handle
{
	u32 uid;
	A_Type type;
};

static inline A_Handle A_HandleNull(void)
{
	A_Handle handle = {0};
	handle.uid = 0;
	handle.type = A_Type_Null;
	
	return handle;
}

static inline b32 A_HandleIsNull(A_Handle handle)
{
	return (handle.uid == 0 ||
			handle.type == A_Type_Null);
}

static inline b32 A_HandleMatch(A_Handle a, A_Handle b)
{
	return (a.uid == b.uid &&
			a.type == b.type);
}

#endif // ASSET_HANDLE_H
