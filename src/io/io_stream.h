#ifndef IO_STREAM_H
#define IO_STREAM_H

typedef struct IO_Stream IO_Stream;
struct IO_Stream
{
	OS_Handle os_handle;
};

internal IO_Stream IO_StreamFromFile(String8 path, OS_FileMode mode);
internal IO_Stream IO_StreamFromMemory(void *memory, u64 bytes);
internal IO_Stream IO_StreamFromConstMemory(const void *memory, u64 bytes);

internal void IO_StreamRead     (const IO_Stream *stream, void *dst, u64 bytes);
internal void IO_StreamWrite    (const IO_Stream *stream, const void *src, u64 bytes);
internal void IO_StreamSeek     (const IO_Stream *stream, i64 offset);

internal void IO_StreamClose    (IO_Stream *stream);

internal i64  IO_StreamPosition (const IO_Stream *stream);
internal u64  IO_StreamSize     (const IO_Stream *stream);

//#define IO_StreamReadArray(stream, type, count) IO_StreamRead((stream), sizeof(type) * (count))

#endif // IO_STREAM_H
