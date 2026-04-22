#ifndef RENDER_BUFFER_RANGE_H
#define RENDER_BUFFER_RANGE_H

typedef struct R_BufferRange R_BufferRange;
struct R_BufferRange
{
	GFX_BufferKey buffer;
	u64 size;
	u64 offset;
};

internal inline void *
R_BufferRangeMap(const R_BufferRange *range, const GFX_Device *device)
{
	return (void *)((u8 *)GFX_DeviceBufferMap(device, range->buffer) + range->offset);
}

internal inline u64
R_BufferRangeAddress(const R_BufferRange *range, const GFX_Device *device)
{
	return GFX_DeviceBufferAddress(device, range->buffer) + range->offset;
}

#endif // RENDER_BUFFER_RANGE_H
