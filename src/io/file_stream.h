#pragma once

#include <string>

#include "stream.h"

namespace mgp
{
	class FileStream : public Stream
	{
	public:
		FileStream(PlatformCore *platform);
		FileStream(PlatformCore *platform, const char *filename, const char *mode);

		FileStream &open(const std::string &filename, const char *mode);
		FileStream &open(const char *filename, const char *mode);

		bool getLine(std::string &str, int32_t &pointer) const;
	};
}
