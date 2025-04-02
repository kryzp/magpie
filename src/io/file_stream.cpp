#include "file_stream.h"

#include "core/platform.h"

using namespace mgp;

FileStream::FileStream(const Platform *platform)
	: Stream(platform)
{
}

FileStream::FileStream(const Platform *platform, const char *filename, const char *mode)
	: Stream(platform)
{
	open(filename, mode);
}

FileStream &FileStream::open(const std::string &filename, const char *mode)
{
	return open(filename.c_str(), mode);
}

FileStream &FileStream::open(const char *filename, const char *mode)
{
	p_stream = p_platform->streamFromFile(filename, mode);
	return *this;
}

bool FileStream::getLine(std::string &str, int32_t &pointer)
{
	str.clear();

	char c;

	do
	{
		read(&c, 1);
		str.push_back(c);

		pointer++;

		if (pointer > getSize())
			return false;

		seek(pointer);
	}
	while (c != '\n');

	return true;
}
