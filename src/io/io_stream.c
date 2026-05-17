
/*
internal IO_Stream
IO_StreamFromFile(String8 path, OS_FileAccess access)
{
	IO_Stream file_stream = {0};
	file_stream.os_handle = osapi->StreamFromFile(path, access);

	return file_stream;
}

internal IO_Stream
IO_StreamFromMemory(void *memory, u64 bytes)
{
	IO_Stream memory_stream = {0};
	memory_stream.os_handle = osapi->StreamFromMemory(memory, bytes);

	return memory_stream;
}

internal IO_Stream
IO_StreamFromConstMemory(const void *memory, u64 bytes)
{
	IO_Stream const_memory_stream = {0};
	const_memory_stream.os_handle = osapi->StreamFromConstMemory(memory, bytes);

	return const_memory_stream;
}

internal void
IO_StreamRead(const IO_Stream *stream, void *dst, u64 bytes)
{
	osapi->StreamRead(stream->os_handle, dst, bytes);
}

internal void
IO_StreamWrite(const IO_Stream *stream, const void *src, u64 bytes)
{
	osapi->StreamWrite(stream->os_handle, src, bytes);
}

internal void
IO_StreamSeek(const IO_Stream *stream, i64 offset)
{
	osapi->StreamSeek(stream->os_handle, offset);
}

internal void
IO_StreamClose(IO_Stream *stream)
{
	osapi->StreamClose(stream->os_handle);
	stream->os_handle = OS_HandleNull();
}

internal i64
IO_StreamPosition(const IO_Stream *stream)
{
	return osapi->StreamPosition(stream->os_handle);
}

internal u64
IO_StreamSize(const IO_Stream *stream)
{
	return osapi->StreamSize(stream->os_handle);
}
*/
