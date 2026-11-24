#ifndef ASSET_HANDLE_H
#define ASSET_HANDLE_H

#include "core/types.h"
#include "container/string.h"

namespace ast {

enum AssetType {
	AssetType_Null,
	AssetType_Texture,
	AssetType_Shader,
	AssetType_Model,
	AssetType_Sound,
	AssetType_Script,
	AssetType_MaxEnum
};

AssetType TypeFromString(const String &string);
const char *StringFromType(AssetType type);

struct Handle {
	unsigned id = 0;
	unsigned generation = 0;
	AssetType type = AssetType_Null;

	bool IsNull() const
	{
		return type == AssetType_Null;
	}

	bool operator==(const Handle &other) const
	{
		return (
			id == other.id &&
			generation == other.generation &&
			type == other.type
		);
	}

	bool operator!=(const Handle &other) const
	{
		return !(*this == other);
	}
};

}

#endif // ASSET_HANDLE_H
