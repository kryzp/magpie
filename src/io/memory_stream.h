#pragma once

#include "stream.h"

namespace mgp
{
	class MemoryStream : public Stream
	{
	public:
		MemoryStream(PlatformCore *platform);
		MemoryStream(PlatformCore *platform, void *memory, uint64_t length);

		MemoryStream &open(void *memory, uint64_t length);
	};

	class ConstMemoryStream : public Stream
	{
	public:
		ConstMemoryStream(PlatformCore *platform);
		ConstMemoryStream(PlatformCore *platform, const void *memory, uint64_t length);

		ConstMemoryStream &open(const void *memory, uint64_t length);
	};
}
