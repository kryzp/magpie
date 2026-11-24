#ifndef GRAPHICS_ACCEL_STRUCT_H
#define GRAPHICS_ACCEL_STRUCT_H

#include "core/types.h"
#include "graphics/resource_key.h"
#include "graphics/mesh.h"

#include <volk/volk.h>

namespace gfx {

struct BLASGeometry {

	// vertices
	ResourceKey vertex_buffer;
	unsigned vertex_count = 0;
	unsigned vertex_stride = 0;
	VkFormat vertex_format = VK_FORMAT_UNDEFINED;
	u64 vertex_offset = 0;

	// indices
	ResourceKey index_buffer;
	unsigned index_count = 0;
	IndexType index_type = IndexType_U32;
	u64 index_offset = 0;
};

struct AccelStruct {
	VkAccelerationStructureKHR vk_handle = VK_NULL_HANDLE;
	VkAccelerationStructureTypeKHR type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	ResourceKey backing_buffer;
	u64 device_address = 0;
};

struct AccelStructReceipt {
	ResourceKey key;
	u64 scratch_size = 0;
};

}

#endif // GRAPHICS_ACCEL_STRUCT_H
