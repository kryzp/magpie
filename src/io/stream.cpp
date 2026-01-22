#include "stream.h"

#include "platform/platform.h"

using namespace io;

Stream::Stream(void *stream)
	: handle(stream)
{
}

Stream::~Stream()
{
	if (handle)
		close();
}

Stream Stream::from_file(const char *path, FileMode mode)
{
	return Stream(platform::stream_from_file(path, file_mode_to_string(mode)));
}

Stream Stream::from_memory(void *memory, uint64_t length)
{
	return Stream(platform::stream_from_memory(memory, length));
}

Stream Stream::from_const_memory(const void *memory, uint64_t length)
{
	return Stream(platform::stream_from_const_memory(memory, length));
}

void Stream::read(void *buffer, u64 length) const
{
	platform::stream_read(handle, buffer, length);
}

void Stream::write(void *data, u64 length) const
{
	platform::stream_write(handle, data, length);
}

void Stream::seek(s64 offset) const
{
	platform::stream_seek(handle, offset);
}

void Stream::close()
{
	platform::stream_close(handle);
	handle = nullptr;
}

s64 Stream::position() const
{
	return platform::stream_position(handle);
}

s64 Stream::size() const
{
	return platform::stream_size(handle);
}

void *Stream::get_handle()
{
	return handle;
}

const void *Stream::get_handle() const
{
	return handle;
}

bool Stream::get_line(String &str, int32_t &pointer) const
{
	str.clear();

	char c;

	do {
		read(&c, 1);
		str.push_back(c);

		pointer++;

		if (pointer > size())
			return false;

		seek(pointer);
	} while(c != '\n' && c != '\n\r');

	return true;
}
