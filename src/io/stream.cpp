#include "stream.h"

#include "core/platform.h"

using namespace mgp;

Stream::Stream(const Platform *platform)
	: p_platform(platform)
	, p_stream(nullptr)
{
}

Stream::~Stream()
{
	close(); // close automatically on destroy.
}

void Stream::read(void *buffer, uint64_t length) const
{
	if (p_stream)
		p_platform->streamRead(p_stream, buffer, length);
}

void Stream::write(void *data, uint64_t length) const
{
	if (p_stream)
		p_platform->streamWrite(p_stream, data, length);
}

void Stream::seek(int64_t offset) const
{
	if (p_stream)
		p_platform->streamSeek(p_stream, offset);
}

void Stream::close()
{
	if (!p_stream)
		return;

	p_platform->streamClose(p_stream);
	p_stream = nullptr;
}

int64_t Stream::getPosition() const
{
	if (p_stream)
		return p_platform->streamPosition(p_stream);

	return -1; // stream isnt open, return -1
}

int64_t Stream::getSize() const
{
	if (p_stream)
		return p_platform->streamSize(p_stream);

	return -1; // stream isnt open, return -1
}

void *Stream::getStream()
{
	return p_stream;
}

const void *Stream::getStream() const
{
	return p_stream;
}
