#pragma once

#include <string>

#include "stream.h"

namespace mgp
{
	/**
	 * File-specialized stream.
	 */
	class FileStream : public Stream
	{
	public:
		FileStream(const Platform *platform);
		FileStream(const Platform *platform, const char *filename, const char *mode);

		FileStream &open(const std::string &filename, const char *mode);
		FileStream &open(const char *filename, const char *mode);

		bool getLine(std::string &str, int32_t &pointer);
	};
}
