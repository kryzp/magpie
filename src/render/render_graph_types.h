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
//           R_Pass *bloom = R_GraphAdd(graph, String8Lit("Bloom"), R_PassType_Compute);
//           R_PassReadTextureCompute(bloom, hdr); // Reads before lighting writes to it!
//                                                    This won't cause a crash but it will be garbage.
//
//           R_Pass *lighting = R_GraphAdd(graph, String8Lit("Lighting"), R_PassType_Graphics);
//           R_PassWriteColour(lighting, hdr, &clear); // Actually writes to it.

// NOTE: 0 is reserved for the invalid handle so indices start at 1.
//       R_GraphCreateTexture allocates version index 1+ for the initial version.

#define R_GRAPH_INVALID_INDEX ((u32)-1)

typedef struct R_GraphTexHandle { u32 value; } R_GraphTexHandle;
typedef struct R_GraphBufHandle { u32 value; } R_GraphBufHandle;

internal inline R_GraphTexHandle R_GraphTexHandleNull(void) { return (R_GraphTexHandle) {0}; }
internal inline R_GraphBufHandle R_GraphBufHandleNull(void) { return (R_GraphBufHandle) {0}; }

internal inline b32 R_GraphTexHandleIsNull(R_GraphTexHandle h) { return h.value == 0; }
internal inline b32 R_GraphBufHandleIsNull(R_GraphBufHandle h) { return h.value == 0; }

internal inline b32 R_GraphTexHandleMatch(R_GraphTexHandle a, R_GraphTexHandle b) { return a.value == b.value; }
internal inline b32 R_GraphBufHandleMatch(R_GraphBufHandle a, R_GraphBufHandle b) { return a.value == b.value; }

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

internal R_TextureInfo R_TextureInfoInit (void);
internal R_BufferInfo  R_BufferInfoInit  (void);

internal b32 R_TextureInfoMatch (const R_TextureInfo *a, const R_TextureInfo *b);
internal b32 R_BufferInfoMatch  (const R_BufferInfo  *a, const R_BufferInfo  *b);

/*
typedef struct R_ResourceState R_ResourceState;
struct R_ResourceState
{
	VkPipelineStageFlags2 stage;
	VkAccessFlags2 to_flush;
	VkAccessFlags2 invalidated_in_stage[64]; // TODO: Could this be represented better somehow.
	VkImageLayout layout;
};
*/

typedef struct R_ResourceState R_ResourceState;
struct R_ResourceState
{
	VkPipelineStageFlags2 write_stage;  // stage of last unsynced write
	VkAccessFlags2        write_access; // WRITE access pending flush (0 = no pending writes)

	VkPipelineStageFlags2 read_stages; // accumulated reader stages since last flush (for WAR)
	
	VkImageLayout layout;
};

#endif // RENDER_GRAPH_TYPES_H
