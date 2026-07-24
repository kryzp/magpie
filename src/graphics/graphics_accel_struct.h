#ifndef GRAPHICS_ACCEL_STRUCT_H
#define GRAPHICS_ACCEL_STRUCT_H

typedef struct G_BLASGeometry G_BLASGeometry;
struct G_BLASGeometry
{
	G_ResourceKey vertex_buffer;
	u32 vertex_count;
	u32 vertex_stride;
	VkFormat vertex_format;
	u64 vertex_offset;

	G_ResourceKey index_buffer;
	u32 index_count;
	G_IndexType index_type;
	u64 index_offset;
};

typedef struct G_AccelStruct G_AccelStruct;
struct G_AccelStruct
{
	VkAccelerationStructureKHR vk_handle;
	VkAccelerationStructureTypeKHR type;
	G_ResourceKey backing_buffer;
	u64 device_address;
};

#endif // GRAPHICS_ACCEL_STRUCT_H
