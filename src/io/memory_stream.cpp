#include "memory_stream.h"

#include "core/platform.h"

using namespace mgp;

MemoryStream::MemoryStream(const Platform *platform)
	: Stream(platform)
{
}

MemoryStream::MemoryStream(const Platform *platform, void *memory, uint64_t length)
	: Stream(platform)
{
	open(memory, length);
}

MemoryStream &MemoryStream::open(void *memory, uint64_t length)
{
	p_stream = p_platform->streamFromMemory(memory, length);
	return *this;
}

ConstMemoryStream::ConstMemoryStream(const Platform *platform)
	: Stream(platform)
{
}

ConstMemoryStream::ConstMemoryStream(const Platform *platform, const void *memory, uint64_t length)
	: Stream(platform)
{
	open(memory, length);
}

ConstMemoryStream &ConstMemoryStream::open(const void *memory, uint64_t length)
{
	p_stream = p_platform->streamFromConstMemory(memory, length);
	return *this;
}
