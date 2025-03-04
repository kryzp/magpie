#include "file_stream.h"

#include "../platform.h"

using namespace llt;

FileStream::FileStream()
	: Stream()
{
}

FileStream::FileStream(const char *filename, const char *mode)
	: Stream()
{
	open(filename, mode);
}

FileStream &FileStream::open(const String &filename, const char *mode)
{
	return open(filename.cstr(), mode);
}

FileStream &FileStream::open(const char *filename, const char *mode)
{
	p_stream = g_platform->streamFromFile(filename, mode);
	return *this;
}

bool FileStream::getLine(String &str, int32_t &pointer)
{
	// clear the current line string
	str.clear();

	char c;
	do
	{
		// read the next character and add it to the current line string
		read(&c, 1);
		str.pushBack(c);

		// move forward
		pointer++;

		// if we hit the end of the file, exit and return false
		// this is because this function is best used in a while() loop, so returning false will automatically exit the loop
		if (pointer > size()) {
			return false;
		}

		// move the stream to the position of our pointer
		seek(pointer);
	}
	while (c != '\n');

	return true;
}
