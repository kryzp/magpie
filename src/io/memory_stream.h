#ifndef MEMORY_STREAM_H_
#define MEMORY_STREAM_H_

#include "stream.h"

namespace llt
{
	/**
	 * Memory-specialized stream.
	 */
	class MemoryStream : public Stream
	{
	public:
		MemoryStream();
		MemoryStream(void *memory, uint64_t length);
		MemoryStream &open(void *memory, uint64_t length);
	};

	/**
	 * Const-memory-specialized stream.
	 */
	class ConstMemoryStream : public Stream
	{
	public:
		ConstMemoryStream();
		ConstMemoryStream(const void *memory, uint64_t length);
		ConstMemoryStream &open(const void *memory, uint64_t length);
	};
}

#endif // MEMORY_STREAM_H_
