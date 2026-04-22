
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
