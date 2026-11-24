#ifndef GRAPHICS_RESOURCE_KEY_H
#define GRAPHICS_RESOURCE_KEY_H

#include "core/types.h"

namespace gfx {
struct ResourceKey {
	u64 value = 0;

	ResourceKey() = default;

	explicit ResourceKey(u64 value_)
		: value(value_)
	{
	}

	bool IsNull() const
	{
		return value == 0;
	}

	bool operator==(const ResourceKey &other) const
	{
		return value == other.value;
	}

	bool operator!=(const ResourceKey &other) const
	{
		return value != other.value;
	}

	static ResourceKey Null()
	{
		return ResourceKey();
	}
};
}

#endif // GRAPHICS_RESOURCE_KEY_H
