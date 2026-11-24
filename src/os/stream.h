#ifndef OS_STREAM_H
#define OS_STREAM_H

#include "core/types.h"

#include "container/string.h"

struct SDL_IOStream;

namespace os {
enum FileAccess : unsigned {
	FileAccess_None = 0,
	FileAccess_Read = 1 << 0,
	FileAccess_Write = 1 << 1,
	FileAccess_CreateIfMissing = 1 << 2,
	FileAccess_OverwriteIfExists = 1 << 3,
	FileAccess_Append = 1 << 4,
	FileAccess_Exclusive = 1 << 5,
	FileAccess_NonBinary = 1 << 6
};

constexpr unsigned FilePresetOpen = FileAccess_Write | FileAccess_CreateIfMissing | FileAccess_Append;
constexpr unsigned FilePresetOpenRW = FileAccess_Write | FileAccess_CreateIfMissing | FileAccess_Append | FileAccess_Read;

constexpr unsigned FilePresetCreate = FileAccess_Write | FileAccess_CreateIfMissing | FileAccess_OverwriteIfExists;
constexpr unsigned FilePresetCreateRW = FileAccess_Write | FileAccess_CreateIfMissing | FileAccess_OverwriteIfExists | FileAccess_Read;

class Stream {
public:
	Stream();
	~Stream();

	Stream(Stream &&other);
	Stream &operator=(Stream &&other);

	Stream(const Stream &) = delete;
	Stream &operator=(const Stream &) = delete;

	static Stream FromFile(const String &path, unsigned access);
	static Stream FromMemory(void *memory, u64 bytes);
	static Stream FromConstMemory(const void *memory, u64 bytes);

	bool IsOpen() const
	{
		return io != nullptr;
	}

	i64 Read(void *dst, u64 bytes);
	i64 Write(const void *src, u64 bytes);
	i64 Seek(i64 offset);
	i64 Size();
	i64 Position();
	bool Flush();
	bool Close();

private:
	explicit Stream(SDL_IOStream *io_);

	SDL_IOStream *io;
};
}

#endif // OS_STREAM_H
