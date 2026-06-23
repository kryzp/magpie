#ifndef RENDER_GEOMETRY_H
#define RENDER_GEOMETRY_H

#define R_GEOMETRY_PAGE_VERTEX_BUFFER_SIZE     Megabytes(64)
#define R_GEOMETRY_PAGE_INDEX_BUFFER_SIZE      Megabytes(32)

#define R_GEOMETRY_FREE_LIST_MAX_BLOCKS        128

typedef struct R_GeometryFreeBlock R_GeometryFreeBlock;
struct R_GeometryFreeBlock
{
	u64 offset;
	u64 capacity;
};

typedef struct R_GeometryFreeList R_GeometryFreeList;
struct R_GeometryFreeList
{
	R_GeometryFreeBlock blocks[R_GEOMETRY_FREE_LIST_MAX_BLOCKS];
	u32 count;
	u64 total_free;
};

static void R_GeometryFreeListInit      (      R_GeometryFreeList *list, u64 capacity);
static b32  R_GeometryFreeListAvailable (const R_GeometryFreeList *list, u64 size);
static b32  R_GeometryFreeListTryAlloc  (      R_GeometryFreeList *list, u64 size, u64 *out_offset);
static void R_GeometryFreeListRelease   (      R_GeometryFreeList *list, u64 offset, u64 size);

typedef struct R_GeometryPage R_GeometryPage;
struct R_GeometryPage
{
	G_BufferKey vertex_buffer;
	G_BufferKey index_buffer;

	u32 vertex_count;
	u32 index_count;
	
	u32 max_vertices;
	u32 max_indices;

	R_GeometryFreeList vertex_free;
	R_GeometryFreeList index_free;
};

#endif // RENDER_GEOMETRY_H
