#ifndef RENDER_BUFFER_RANGE_H
#define RENDER_BUFFER_RANGE_H

typedef struct R_BufferRange R_BufferRange;
struct R_BufferRange
{
	G_BufferKey buffer;
	u64 size;
	u64 offset;
};

static inline void *R_BufferRangeMap(const R_BufferRange *range)
{
	return (void *)((u8 *)G_DeviceBufferMap(range->buffer) + range->offset);
}

static inline u64 R_BufferRangeAddress(const R_BufferRange *range)
{
	return G_DeviceBufferAddress(range->buffer) + range->offset;
}

#endif // RENDER_BUFFER_RANGE_H
