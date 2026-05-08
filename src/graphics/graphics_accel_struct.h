#ifndef GRAPHICS_ACCEL_STRUCT_H
#define GRAPHICS_ACCEL_STRUCT_H

typedef struct GFX_BLASGeometry GFX_BLASGeometry;
struct GFX_BLASGeometry
{
	GFX_BufferKey vertex_buffer;
	u32 vertex_count;
	u32 vertex_stride;
	VkFormat vertex_format;
	u64 vertex_offset;

	GFX_BufferKey index_buffer;
	u32 index_count;
	VkIndexType index_type;
	u64 index_offset;
};

typedef struct GFX_AccelStruct GFX_AccelStruct;
struct GFX_AccelStruct
{
	VkAccelerationStructureKHR vk_handle;
	VkAccelerationStructureTypeKHR type;
	GFX_BufferKey backing_buffer;
	u64 device_address;
};

#endif // GRAPHICS_ACCEL_STRUCT_H
