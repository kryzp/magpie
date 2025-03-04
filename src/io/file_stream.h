#ifndef FILE_STREAM_H_
#define FILE_STREAM_H_

#include "stream.h"
#include "../container/string.h"

namespace llt
{
	/**
	 * File-specialized stream.
	 */
	class FileStream : public Stream
	{
	public:
		FileStream();
		FileStream(const char *filename, const char *mode);

		FileStream &open(const String &filename, const char *mode);
		FileStream &open(const char *filename, const char *mode);

		/*
		 * Allows for iterating through each line in a file one-by-one.
		 * Requires a variable "pointer" to be created beforehand and referenced
		 * to cache the current line.
		 */
		bool getLine(String &str, int32_t &pointer);
	};
}

#endif // FILE_STREAM_H_
