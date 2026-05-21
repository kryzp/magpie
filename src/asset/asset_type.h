#ifndef ASSET_TYPE_H
#define ASSET_TYPE_H

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
	
	DebugPrintB("Unknown Asset Name: %.*s", String8VArg(str));

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

#endif // ASSET_TYPE_H
