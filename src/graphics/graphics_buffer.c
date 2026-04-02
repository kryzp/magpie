
internal void
GFX_BufferRead(const GFX_Buffer *buffer, void *dst, u64 length, u64 offset)
{
	vmaCopyAllocationToMemory(buffer->allocator, buffer->allocation, offset, dst, length);
}

internal void
GFX_BufferWrite(const GFX_Buffer *buffer, const void *src, u64 length, u64 offset)
{
	vmaCopyMemoryToAllocation(buffer->allocator, src, buffer->allocation, offset, length);
}

internal void *
GFX_BufferMap(const GFX_Buffer *buffer)
{
	return buffer->allocation_info.pMappedData;
}

internal u64
GFX_BufferAddress(const GFX_Buffer *buffer)
{
	return buffer->device_address;
}

internal b32
GFX_BufferIsStorage(const GFX_Buffer *buffer)
{
	return (buffer->usage & VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT) != 0;
}

internal b32
GFX_BufferIsUniform(const GFX_Buffer *buffer)
{
	return (buffer->usage & VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT) != 0;
}
