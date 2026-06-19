#ifndef ASSET_TYPE_H
#define ASSET_TYPE_H

typedef enum A_Type
{
	A_Type_Null,
#define AssetDef(name, upper) A_Type_##name,
#include "asset_definitions.inc"
#undef AssetDef
	A_Type_COUNT
}
A_Type;

internal inline A_Type
A_TypeFromString(String8 str)
{
#define AssetDef(name, upper) if (String8Match(str, String8Lit(#name))) return A_Type_##name;
#include "asset_definitions.inc"
#undef AssetDef
	
	DebugPrintB("Unknown Asset Name: %.*s", String8VArg(str));

	return A_Type_COUNT;
}

internal inline String8
A_StringFromType(Arena *arena, A_Type type)
{
#define AssetDef(name, upper) if (type == A_Type_##name) return String8Lit(#name);
#include "asset_definitions.inc"
#undef AssetDef
	
	DebugPrintB("Unknown Asset Type: %d", type);

	return (String8) {0};
}

#endif // ASSET_TYPE_H
