#include "memory_stream.h"

#include "platform/platform_core.h"

using namespace mgp;

MemoryStream::MemoryStream(PlatformCore *platform)
	: Stream(platform)
{
}

MemoryStream::MemoryStream(PlatformCore *platform, void *memory, uint64_t length)
	: Stream(platform)
{
	open(memory, length);
}

MemoryStream &MemoryStream::open(void *memory, uint64_t length)
{
	p_stream = p_platform->streamFromMemory(memory, length);
	return *this;
}

ConstMemoryStream::ConstMemoryStream(PlatformCore *platform)
	: Stream(platform)
{
}

ConstMemoryStream::ConstMemoryStream(PlatformCore *platform, const void *memory, uint64_t length)
	: Stream(platform)
{
	open(memory, length);
}

ConstMemoryStream &ConstMemoryStream::open(const void *memory, uint64_t length)
{
	p_stream = p_platform->streamFromConstMemory(memory, length);
	return *this;
}
