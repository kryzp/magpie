#ifndef ASSET_HANDLE_H
#define ASSET_HANDLE_H

#include "core/core_types.h"

struct asset_handle {
	u32 index;
};

static inline bool asset_handles_equal(struct asset_handle a, struct asset_handle b)
{
	return a.index == b.index;
}

#endif // ASSET_HANDLE_H
