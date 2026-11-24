#include "asset/handle.h"
#include "core/log.h"

static const char *type_names[] = {
	"Null",
	"Texture",
	"Shader",
	"Model",
	"Sound",
	"Script"
};

static_assert(ArraySize(type_names) == ast::AssetType_MaxEnum, "Missing asset type name.");

ast::AssetType ast::TypeFromString(const String &string)
{
	for (unsigned i = AssetType_Null + 1; i < AssetType_MaxEnum; i++) {
		if (string == type_names[i])
			return (AssetType)i;
	}

	DebugPrintB("Unknown Asset Name: %s", string.c_str());

	return AssetType_Null;
}

const char *ast::StringFromType(AssetType type)
{
	if (type < AssetType_MaxEnum)
		return type_names[type];

	DebugPrintB("Unknown Asset Type: %d", (int)type);

	return "";
}
