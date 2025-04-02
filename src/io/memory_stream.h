#pragma once

#include "stream.h"

namespace mgp
{
	/**
	 * Memory-specialized stream.
	 */
	class MemoryStream : public Stream
	{
	public:
		MemoryStream(const Platform *platform);
		MemoryStream(const Platform *platform, void *memory, uint64_t length);

		MemoryStream &open(void *memory, uint64_t length);
	};

	/**
	 * Const-memory-specialized stream.
	 */
	class ConstMemoryStream : public Stream
	{
	public:
		ConstMemoryStream(const Platform *platform);
		ConstMemoryStream(const Platform *platform, const void *memory, uint64_t length);

		ConstMemoryStream &open(const void *memory, uint64_t length);
	};
}
