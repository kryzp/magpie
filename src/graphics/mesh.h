#ifndef GRAPHICS_MESH_H
#define GRAPHICS_MESH_H

#include "core/types.h"
#include "core/log.h"

#include <volk/volk.h>

namespace gfx {

enum IndexType {
	IndexType_U16,
	IndexType_U32,
	IndexType_MaxEnum
};

inline VkIndexType IndexTypeToVk(IndexType type)
{
	switch (type) {
		case IndexType_U16:
			return VK_INDEX_TYPE_UINT16;
		case IndexType_U32:
			return VK_INDEX_TYPE_UINT32;
		default:
			break;
	}

	AssertTrue(false);

	return static_cast<VkIndexType>(0);
}

inline u64 IndexTypeStride(IndexType type)
{
	switch (type) {
		case IndexType_U16:
			return sizeof(u16);
		case IndexType_U32:
			return sizeof(unsigned);
		default:
			break;
	}

	AssertTrue(false);

	return 0;
}

}

#endif // GRAPHICS_MESH_H
