
internal b32 G_BufferIsStorage(const G_Buffer *buffer)
{
	return (buffer->usage & VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT) != 0;
}

internal b32 G_BufferIsUniform(const G_Buffer *buffer)
{
	return (buffer->usage & VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT) != 0;
}
