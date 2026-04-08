#ifndef RENDER_GRAPH_TYPES_H
#define RENDER_GRAPH_TYPES_H

// NOTE: Yes, we could just use one R_GraphHandle here, and everything
//       would work, in some cases it would even make code cleaner,
//       but it would also remove type safety and I don't wanna bother
//       with debugging the fact I accidentally passed a buffer into
//       a texture slot and vice versa. It also makes it obvious when
//       reading code what resource we are talking about.

// NOTE: These index into the graph's version table because in my
//       implementation pass decleration order != pass execution order.
//       It also helps prevent bugs like reading a texture before
//       writing to it, which won't cause a crash but it will
//       result in complete garbage data being rendered.
//
//           R_Pass *bloom = R_GraphAdd(graph, Str8("Bloom"), R_PassType_Compute);
//           R_PassReadTextureCompute(bloom, hdr); // Reads before lighting writes to it!
//                                                    This won't cause a crash but it will be garbage.
//
//           R_Pass *lighting = R_GraphAdd(graph, Str8("Lighting"), R_PassType_Graphics);
//           R_PassWriteColour(lighting, hdr, &clear); // Actually writes to it.

typedef struct R_GraphTexHandle { u32 value; } R_GraphTexHandle;
typedef struct R_GraphBufHandle { u32 value; } R_GraphBufHandle;

inline internal R_GraphTexHandle
R_GraphTexHandleNull(void)
{
	R_GraphTexHandle null_handle = {0};
	return null_handle;
}

inline internal R_GraphBufHandle
R_GraphBufHandleNull(void)
{
	R_GraphBufHandle null_handle = {0};
	return null_handle;
}

typedef enum R_SizeClass
{
	R_SizeClass_Absolute,
	R_SizeClass_SwapchainRelative,
	R_SizeClass_COUNT
}
R_SizeClass;

typedef struct R_TextureInfo R_TextureInfo;
struct R_TextureInfo
{
	VkFormat format;

	R_SizeClass size_class;
	
	f32 size_x;
	f32 size_y;
	f32 size_z;

	u32 mips;
	u32 layers;
	
	u32 samples;

	GFX_TextureAllocFlags flags;
};

typedef struct R_BufferInfo R_BufferInfo;
struct R_BufferInfo
{
	VkDeviceSize size;
	VmaAllocationCreateFlags flags;
	VkBufferUsageFlags2 usage;
};

internal R_TextureInfo R_TextureInfoInit(void);
internal R_BufferInfo R_BufferInfoInit(void);

internal b32 R_TextureInfoMatch(const R_TextureInfo *a, const R_TextureInfo *b);
internal b32 R_BufferInfoMatch(const R_BufferInfo *a, const R_BufferInfo *b);

typedef struct R_ResourceState R_ResourceState;
struct R_ResourceState
{
	VkPipelineStageFlags2 stage;

	// Access masks that need to be made available (flushed).
	VkAccessFlags2 to_flush;

	// Per-Stage visibility bitmask of stages.
	// TODO: Could this be represented better somehow.
	VkAccessFlags2 invalidated_in_stage[64]; 

	// For textures only.
	// Start off as VK_IMAGE_LAYOUT_UNDEFINED (0).
	VkImageLayout layout;
};

#endif // RENDER_GRAPH_TYPES_H
