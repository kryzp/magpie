#ifndef RENDER_BUFFER_RANGE_H
#define RENDER_BUFFER_RANGE_H

typedef struct R_BufferRange R_BufferRange;
struct R_BufferRange
{
	G_BufferKey buffer;
	u64 size;
	u64 offset;
};

internal inline void *
R_BufferRangeMap(const R_BufferRange *range, const G_Device *device)
{
	return (void *)((u8 *)G_DeviceBufferMap(device, range->buffer) + range->offset);
}

internal inline u64
R_BufferRangeAddress(const R_BufferRange *range, const G_Device *device)
{
	return G_DeviceBufferAddress(device, range->buffer) + range->offset;
}

#endif // RENDER_BUFFER_RANGE_H
