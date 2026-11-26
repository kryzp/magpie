#include "os/stream.h"
#include "core/log.h"

#include <SDL3/SDL.h>

#include <cstdio>

os::Stream::Stream()
	: io(nullptr)
{
}

os::Stream::Stream(SDL_IOStream *io_)
	: io(io_)
{
}

os::Stream::~Stream()
{
	Close();
}

os::Stream::Stream(Stream &&other)
	: io(other.io)
{
	other.io = nullptr;
}

os::Stream &os::Stream::operator=(Stream &&other)
{
	if (this != &other) {
		Close();
		io = other.io;
		other.io = nullptr;
	}

	return *this;
}

os::Stream os::Stream::FromFile(const String &path, unsigned access)
{
	bool read = access & FileAccess_Read;
	bool write = access & FileAccess_Write;
	bool create = access & FileAccess_CreateIfMissing;
	bool overwrite = access & FileAccess_OverwriteIfExists;
	bool append = access & FileAccess_Append;
	bool excl = access & FileAccess_Exclusive;
	bool non_binary = access & FileAccess_NonBinary;

	AssertTrue(read || write);

	const char *bin = non_binary ? "" : "b";

	char mode[8] = {};

	// 'x' (exclusive) has to come last to be valid.
	if (append)
		snprintf(mode, sizeof(mode), "a%s%s", read ? "+" : "", bin);
	else if (overwrite)
		snprintf(mode, sizeof(mode), "w%s%s%s", read ? "+" : "", bin, excl ? "x" : "");
	else
		snprintf(mode, sizeof(mode), "%s%s", write ? "r+" : "r", bin);

	SDL_IOStream *io = SDL_IOFromFile(path.c_str(), mode);

	if (!io && create && !append && !overwrite) {
		snprintf(mode, sizeof(mode), "w+%s", bin);
		io = SDL_IOFromFile(path.c_str(), mode);
	}

	return Stream(io);
}

os::Stream os::Stream::FromMemory(void *memory, u64 bytes)
{
	return Stream(SDL_IOFromMem(memory, bytes));
}

os::Stream os::Stream::FromConstMemory(const void *memory, u64 bytes)
{
	return Stream(SDL_IOFromConstMem(memory, bytes));
}

i64 os::Stream::Read(void *dst, u64 bytes)
{
	return static_cast<i64>(SDL_ReadIO(io, dst, bytes));
}

i64 os::Stream::Write(const void *src, u64 bytes)
{
	return static_cast<i64>(SDL_WriteIO(io, src, bytes));
}

i64 os::Stream::Seek(i64 offset)
{
	return SDL_SeekIO(io, offset, SDL_IO_SEEK_SET);
}

i64 os::Stream::Size()
{
	return SDL_GetIOSize(io);
}

i64 os::Stream::Position()
{
	return SDL_TellIO(io);
}

bool os::Stream::Flush()
{
	return SDL_FlushIO(io);
}

bool os::Stream::Close()
{
	if (!io)
		return false;

	bool result = SDL_CloseIO(io);
	io = nullptr;

	return result;
}
